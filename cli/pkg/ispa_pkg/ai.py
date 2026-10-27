"""Interactive AI assisted LL recovery for ISPA grammars.

ai.json uses byte offsets and an exact source spelling to reject stale
suggestions after a grammar edit.
"""

import argparse
import getpass
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import time
from urllib.error import HTTPError, URLError
from urllib.parse import urlparse
from urllib.request import Request, urlopen


IDENT = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")
RULE = re.compile(r"(?m)^[ \t]*#?(?:\[\w+\][ \t]*)?([A-Za-z_]\w*)[ \t]*:")
ADVANCED_RULE = re.compile(r"(?m)^[ \t]*[A-Za-z_]\w*(?:<[^>\n]*>)?[ \t]*\{")
KINDS = {"insert", "replace", "sync_tokens"}
OUTPUT_FILE = "ai.json"
LEGACY_FILE = "ai.gle"
COPILOT_CHAT_URL = "https://api.githubcopilot.com/chat/completions"
COPILOT_TOKEN_URL = "https://api.github.com/copilot_internal/v2/token"
COPILOT_MODELS_URL = "https://api.githubcopilot.com/models"
COPILOT_CLIENT_ID = "Iv1.b507a08c87ecfe98"
_copilot_cache = {}


class IncompleteResponse(ValueError):
    """The provider stopped before completing a JSON answer."""


GROQ_SCHEMA = {
    "type": "object", "properties": {
        "suggestions": {"type": "array", "items": {
            "type": "object", "properties": {
                "id": {"type": "string"}, "message": {"type": "string"},
                "strategy": {"type": "string", "enum": sorted(KINDS)},
                "sync": {"type": "array", "items": {"type": "string"}},
            }, "required": ["id", "message", "strategy", "sync"],
            "additionalProperties": False,
        }},
    }, "required": ["suggestions"], "additionalProperties": False,
}


def config_path():
    base = Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local/share"))
    return base / "ispa" / ".env"


def read_env(path):
    values = {}
    if path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            if not line or line.lstrip().startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return values


def effective_config(values):
    """Recognize an older OpenAI setup pointed at Copilot's token endpoint."""
    config = dict(values)
    if config.get("ISPA_AI_PROVIDER") == "openai" and config.get("ISPA_AI_URL", "").rstrip("/") == COPILOT_TOKEN_URL:
        config["ISPA_AI_PROVIDER"] = "copilot"
        config["ISPA_AI_URL"] = COPILOT_CHAT_URL
        config["ISPA_AI_GITHUB_TOKEN"] = config.pop("ISPA_AI_API_KEY", "")
        config["ISPA_AI_MODEL"] = ""
    return config


def save_env(path, values):
    path.parent.mkdir(parents=True, exist_ok=True)
    data = "".join(f"{key}={value}\n" for key, value in values.items())
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            stream.write(data)
    finally:
        if os.name == "posix":
            path.chmod(0o600)


def configure(path):
    current = effective_config(read_env(path))
    print(f"Configuration: {path}")
    print(f"Provider: {current.get('ISPA_AI_PROVIDER', '(none)')}; model: {current.get('ISPA_AI_MODEL', '(none)')}")
    credential = current.get("ISPA_AI_GITHUB_TOKEN") if current.get("ISPA_AI_PROVIDER") == "copilot" else current.get("ISPA_AI_API_KEY")
    print(f"Credential: {'configured' if credential else 'not configured'}")
    choice = input("Provider [anthropic/openai/ollama/copilot/copilot-proxy, Enter to keep]: ").strip().lower()
    if not choice:
        if not current.get("ISPA_AI_PROVIDER"):
            raise ValueError("Configure a provider first")
        return current
    if choice not in {"anthropic", "openai", "ollama", "copilot", "copilot-proxy"}:
        raise ValueError("Choose anthropic, openai, ollama, copilot, or copilot-proxy")
    if choice == "copilot":
        previous = current.get("ISPA_AI_PROVIDER")
        current["ISPA_AI_PROVIDER"] = "copilot"
        current["ISPA_AI_URL"] = COPILOT_CHAT_URL
        current["ISPA_AI_MODEL"] = input("Copilot model ID (Enter for default): ").strip()
        current.pop("ISPA_AI_API_KEY", None)
        if previous != "copilot":
            current.pop("ISPA_AI_GITHUB_TOKEN", None)
        key = getpass.getpass("GitHub OAuth access token (Enter to keep existing or start device login): ").strip()
        if key:
            current["ISPA_AI_GITHUB_TOKEN"] = key
        if not current.get("ISPA_AI_GITHUB_TOKEN"):
            current["ISPA_AI_GITHUB_TOKEN"] = github_device_login()
        save_env(path, current)
        return current
    if choice == "copilot-proxy":
        previous = current.get("ISPA_AI_PROVIDER")
        current["ISPA_AI_PROVIDER"] = choice
        current["ISPA_AI_URL"] = input("Proxy chat URL [http://127.0.0.1:5000/v1/chat/completions]: ").strip() or "http://127.0.0.1:5000/v1/chat/completions"
        current["ISPA_AI_MODEL"] = input("Model ID (Enter for proxy default): ").strip()
        current.pop("ISPA_AI_GITHUB_TOKEN", None)
        if previous != choice:
            current.pop("ISPA_AI_API_KEY", None)
        key = getpass.getpass("Proxy API key (optional, Enter to keep existing): ").strip()
        if key:
            current["ISPA_AI_API_KEY"] = key
        save_env(path, current)
        return current
    defaults = {"anthropic": "https://api.anthropic.com/v1/messages",
                "openai": "https://api.openai.com/v1/chat/completions",
                "ollama": "http://localhost:11434/v1/chat/completions"}
    previous = current.get("ISPA_AI_PROVIDER")
    current["ISPA_AI_PROVIDER"] = choice
    current.pop("ISPA_AI_GITHUB_TOKEN", None)
    if previous != choice:
        current.pop("ISPA_AI_API_KEY", None)
    current["ISPA_AI_URL"] = input(f"API URL [{defaults[choice]}]: ").strip() or defaults[choice]
    current["ISPA_AI_MODEL"] = input("Model name: ").strip()
    if not current["ISPA_AI_MODEL"]:
        raise ValueError("Model name is required")
    if choice == "ollama":
        current.pop("ISPA_AI_API_KEY", None)
    else:
        key = getpass.getpass("API key (Enter to keep existing): ").strip()
        if key:
            current["ISPA_AI_API_KEY"] = key
        if not current.get("ISPA_AI_API_KEY"):
            raise ValueError("API key is required")
    save_env(path, current)
    return current


def request_json(request, secrets=(), timeout=90):
    try:
        with urlopen(request, timeout=timeout) as response:
            return json.load(response)
    except HTTPError as error:
        response = error.read(4096).decode(errors="replace")
        if error.code == 403 and re.search(r"(?:Error|error code)\s*:?[ ]*1010\b", response):
            detail = "Cloudflare Error 1010 blocked this client's request signature"
        else:
            try:
                payload = json.loads(response)
                detail = payload.get("error", {}).get("message", "Request rejected")
            except (ValueError, AttributeError):
                detail = "Request rejected by provider or its edge service"
        if not isinstance(detail, str):
            detail = "Request rejected"
        for secret in secrets:
            if secret:
                detail = detail.replace(secret, "[redacted credential]")
        raise ValueError(f"Provider HTTP {error.code}: {detail}") from error
    except URLError as error:
        raise ValueError(f"Provider unavailable: {error.reason}") from error


def github_device_login():
    request = Request("https://github.com/login/device/code",
                      data=json.dumps({"client_id": COPILOT_CLIENT_ID, "scope": "read:user"}).encode(),
                      headers={"Accept": "application/json", "Content-Type": "application/json",
                               "User-Agent": "GithubCopilot/1.155.0"}, method="POST")
    device = request_json(request)
    print(f"Open {device['verification_uri']} and enter code {device['user_code']}")
    interval = max(1, int(device.get("interval", 5)))
    deadline = time.time() + int(device.get("expires_in", 900))
    while time.time() < deadline:
        time.sleep(interval)
        request = Request("https://github.com/login/oauth/access_token",
                          data=json.dumps({"client_id": COPILOT_CLIENT_ID,
                                           "device_code": device["device_code"],
                                           "grant_type": "urn:ietf:params:oauth:grant-type:device_code"}).encode(),
                          headers={"Accept": "application/json", "Content-Type": "application/json",
                                   "User-Agent": "ispa-ai/0.1"}, method="POST")
        result = request_json(request)
        if result.get("access_token"):
            return result["access_token"]
        if result.get("error") == "slow_down":
            interval += 5
        elif result.get("error") != "authorization_pending":
            raise ValueError(f"GitHub authorization failed: {result.get('error', 'unknown response')}")
    raise ValueError("GitHub device authorization expired; configure Copilot again")


def copilot_token(config):
    github_token = config.get("ISPA_AI_GITHUB_TOKEN", "")
    if not github_token:
        raise ValueError("Copilot needs a GitHub OAuth access token; run --configure")
    identity = hashlib.sha256(github_token.encode()).digest()
    cached = _copilot_cache.get(identity)
    if cached and cached[1] > time.time() + 60:
        return cached[0]
    request = Request(COPILOT_TOKEN_URL,
                      headers={"Authorization": f"token {github_token}",
                               "Accept": "application/json", "User-Agent": "ispa-ai/0.1"})
    result = request_json(request, (github_token,))
    token = result.get("token")
    if not token:
        raise ValueError("GitHub did not return a Copilot token; check your Copilot access")
    expires = float(result.get("expires_at") or time.time() + int(result.get("refresh_in", 1200)))
    _copilot_cache[identity] = (token, expires)
    return token


def copilot_models(config):
    token = copilot_token(config)
    request = Request(COPILOT_MODELS_URL,
                      headers={"Authorization": f"Bearer {token}",
                               "Copilot-Integration-Id": "vscode-chat",
                               "Accept": "application/json", "User-Agent": "ispa-ai/0.1"})
    data = request_json(request, (token,))
    return [model["id"] for model in data.get("data", []) if isinstance(model, dict) and model.get("id")]


def _skip(source, i):
    """Skip comments, quoted strings, and Python action blocks."""
    n = len(source)
    if source.startswith("//", i) or source[i] == "#" and (i == 0 or source[i-1] == "\n"):
        j = source.find("\n", i)
        return n if j < 0 else j
    if source[i] in "\"'":
        quote = source[i]
        j = i + 1
        while j < n:
            if source[j] == "\\":
                j += 2
            elif source[j] == quote:
                return j + 1
            else:
                j += 1
        return n
    return i + 1


def _block_end(source, opening):
    """Find the end of a braced ISPA block, ignoring Python text literals."""
    depth = 0
    i = opening
    while i < len(source):
        if source.startswith("//", i) or source.startswith("/*", i):
            marker = "\n" if source.startswith("//", i) else "*/"
            end = source.find(marker, i + 2)
            i = len(source) if end < 0 else end + len(marker)
            continue
        if source[i] == "#":
            end = source.find("\n", i)
            i = len(source) if end < 0 else end
            continue
        if source[i] in "\"'":
            i = _skip(source, i)
            continue
        if source[i] == "[":
            end = source.find("]", i + 1)
            i = len(source) if end < 0 else end + 1
            continue
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return len(source)


def _ignored_blocks(source):
    spans = []
    for pattern in (ADVANCED_RULE, re.compile(r"(?m)^fail\s+[\w.]+\s*\{")):
        for block in pattern.finditer(source):
            if any(start <= block.start() < stop for start, stop in spans):
                continue
            opening = source.find("{", block.start(), block.end())
            spans.append((block.start(), _block_end(source, opening)))
    return sorted(spans)


def scan(root):
    sites = []
    occurrences = {}
    for path in sorted(root.rglob("*.isc")):
        if any(part in {"bootstrapped", "build", "incremental"} for part in path.relative_to(root).parts):
            continue
        source = path.read_text(encoding="utf-8")
        # Rule heads are line anchored. Ignore Python code inside braced blocks.
        heads = list(RULE.finditer(source))
        ignored_spans = _ignored_blocks(source)
        for index, head in enumerate(heads):
            if any(start <= head.start() < stop for start, stop in ignored_spans):
                continue
            name = head.group(1)
            if source[max(0, head.start()-20):head.start()].rstrip().endswith("fail"):
                continue
            end = heads[index+1].start() if index+1 < len(heads) else len(source)
            body = source[head.end():end]
            if "match " in body[:100] or "depends " in body[:100]:
                continue
            i = head.end()
            angle_depth = 0
            while i < end:
                c = source[i]
                if c == ";":
                    break
                if c == "{":
                    depth = 1
                    i += 1
                    while i < end and depth:
                        if source[i] in "\"'":
                            i = _skip(source, i)
                            continue
                        depth += (source[i] == "{") - (source[i] == "}")
                        i += 1
                    continue
                if c == "[":
                    i += 1
                    while i < end and source[i] != "]":
                        i += 2 if source[i] == "\\" else 1
                    i += 1
                    continue
                if c == "<":
                    angle_depth += 1
                    i += 1
                    continue
                if c == ">" and angle_depth:
                    angle_depth -= 1
                    i += 1
                    continue
                if c.isspace() or c in "();|?*+,@&\\]}":
                    i += 1
                    continue
                if source.startswith("//", i):
                    i = _skip(source, i)
                    continue
                if c in "\"'":
                    start = i
                    i = _skip(source, i)
                    spelling = source[start:i]
                    kind = "literal"
                elif c == "%":
                    # An existing user fail marker takes precedence at this site.
                    i += 1
                    m = IDENT.match(source, i)
                    if m:
                        i = m.end()
                    continue
                else:
                    m = IDENT.match(source, i)
                    if not m:
                        i += 1
                        continue
                    start, i = m.start(), m.end()
                    spelling = m.group()
                    kind = "token" if spelling.isupper() else "rule"
                    if spelling in {"fail", "module", "import", "feature", "match", "depends"}:
                        continue
                if angle_depth:
                    continue
                if any(begin <= start < stop for begin, stop in ignored_spans):
                    continue
                previous = source[:start].rstrip()[-1:]
                if previous in {"#", ".", "\\"}:
                    continue
                prefix = source[max(head.end(), start-24):start]
                if re.search(r"%\s*(?:[\w.]+\s*)?$", prefix):
                    continue
                if prefix.rstrip().endswith("|"):
                    continue
                if source[i:end].lstrip().startswith(("?", "*")):
                    continue
                offset = len(source[:start].encode("utf-8"))
                identity = (path.relative_to(root).as_posix(), name, spelling)
                ordinal = occurrences.get(identity, 0)
                occurrences[identity] = ordinal + 1
                fingerprint = hashlib.sha256(f"{identity}:{ordinal}".encode()).hexdigest()[:16]
                sites.append({"id": fingerprint, "file": path.relative_to(root).as_posix(),
                              "rule": name, "symbol": spelling, "kind": kind,
                              "offset": offset, "expected": spelling,
                              "context": source[head.start():min(end, head.start()+400)].strip()})
    return sites


def ask_ai(config, prompt):
    config = effective_config(config)
    provider = config["ISPA_AI_PROVIDER"]
    url = config["ISPA_AI_URL"]
    key = config.get("ISPA_AI_API_KEY", "")
    if provider == "anthropic":
        headers = {"x-api-key": key, "anthropic-version": "2023-06-01"}
        body = {"model": config["ISPA_AI_MODEL"], "max_tokens": 4096,
                "messages": [{"role": "user", "content": prompt}]}
    elif provider == "copilot":
        key = copilot_token(config)
        url = COPILOT_CHAT_URL
        headers = {"Authorization": f"Bearer {key}", "Copilot-Integration-Id": "vscode-chat"}
        body = {"messages": [{"role": "user", "content": prompt}],
                "max_tokens": 4096, "stream": False}
        if config.get("ISPA_AI_MODEL"):
            body["model"] = config["ISPA_AI_MODEL"]
    else:
        headers = {"Authorization": f"Bearer {key}"} if key else {}
        body = {"messages": [{"role": "user", "content": prompt}]}
        if config.get("ISPA_AI_MODEL"):
            body["model"] = config["ISPA_AI_MODEL"]
        if urlparse(url).hostname == "api.groq.com":
            body["max_completion_tokens"] = 4096
            if config["ISPA_AI_MODEL"] in {"openai/gpt-oss-20b", "openai/gpt-oss-120b"}:
                body["reasoning_effort"] = "low"
                body["response_format"] = {
                    "type": "json_schema", "json_schema": {
                        "name": "ispa_recovery_suggestions", "strict": True,
                        "schema": GROQ_SCHEMA,
                    },
                }
    headers["Content-Type"] = "application/json"
    headers["User-Agent"] = "ispa-ai/0.1"
    request = Request(url, data=json.dumps(body).encode(), headers=headers, method="POST")
    result = request_json(request, (key, config.get("ISPA_AI_GITHUB_TOKEN", "")))
    if provider == "anthropic":
        return "".join(part.get("text", "") for part in result.get("content", []))
    choice = result["choices"][0]
    if choice.get("finish_reason") == "length":
        raise IncompleteResponse("Provider reached its output token limit")
    content = choice["message"].get("content")
    if not content:
        raise IncompleteResponse("Provider returned no answer text")
    return content if isinstance(content, str) else "".join(part.get("text", "") for part in content)


def parse_answer(answer, sites):
    answer = answer.strip()
    if answer.startswith("```"):
        answer = re.sub(r"^```(?:json)?\s*|\s*```$", "", answer)
    try:
        data = json.loads(answer)
    except json.JSONDecodeError as error:
        raise IncompleteResponse("Provider returned incomplete or invalid JSON") from error
    if isinstance(data, dict):
        data = data.get("suggestions", [])
    if not isinstance(data, list):
        raise ValueError("AI response must be a JSON array of suggestions")
    allowed = {site["id"]: site for site in sites}
    output = []
    seen = set()
    for item in data:
        if not isinstance(item, dict) or item.get("id") not in allowed or item["id"] in seen:
            continue
        site = allowed[item["id"]]
        message = item.get("message")
        strategy = item.get("strategy")
        sync = item.get("sync", [])
        if not isinstance(message, str) or not message.strip() or len(message) > 500:
            continue
        if strategy not in KINDS or not isinstance(sync, list) or any(not isinstance(x, str) or not IDENT.fullmatch(x) for x in sync):
            continue
        if site["rule"][:1].isupper() and strategy != "insert":
            continue
        if strategy == "sync_tokens" and not sync:
            continue
        output.append({key: site[key] for key in ("id", "file", "rule", "symbol", "offset", "expected")}
                      | {"message": message.strip(), "strategy": strategy, "sync": sync})
        seen.add(item["id"])
    return output


def prompt_for(sites, instruction=""):
    compact = [{key: site[key] for key in ("id", "rule", "symbol", "kind", "context")} for site in sites]
    return ("You design helpful syntax diagnostics for an ISPA LL parser. Each site is a grammar "
            "symbol occurrence. Return only JSON: {\"suggestions\":[{\"id\":site ID,\"message\":"
            "human error message,\"strategy\":\"insert\"|\"replace\"|\"sync_tokens\","
            "\"sync\":[token names]}]}. Every suggestion must explicitly choose a strategy. "
            "Omit a site if it is optional or if no safe custom repair is clear. "
            "Choose insert only when the expected symbol can safely be missing, as with a closing delimiter at EOF. "
            "Choose replace only when one unexpected token can safely be consumed; this drops that token. "
            "Choose sync_tokens for a broken construct that can skip to a known lexer token boundary, "
            "then accept the missing symbol. Put those boundary token names in sync. "
            "Inside uppercase lexer rules, only insert is supported; omit the site if insertion is unsafe. "
            "Do not choose strategies to create variety; choose based on what input may follow the failure. "
            "Do not invent token names or suggest a recovery just to fill the list. "
            f"{instruction}\nSites: {json.dumps(compact, ensure_ascii=False)}")


def suggest_chunk(config, sites):
    try:
        return parse_answer(ask_ai(config, prompt_for(sites)), sites)
    except IncompleteResponse:
        if len(sites) == 1:
            raise ValueError("Provider could not complete a one-site response; try another model")
        middle = len(sites) // 2
        print(f"Provider answer incomplete; retrying groups of {middle} and {len(sites)-middle} sites")
        return suggest_chunk(config, sites[:middle]) + suggest_chunk(config, sites[middle:])


def load_json(path, default):
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() and path.stat().st_size else default


def review(root, sites, suggestions, config, cache):
    by_id = {site["id"]: site for site in sites}
    while True:
        print("\nSuggestions:")
        for i, item in enumerate(suggestions, 1):
            print(f"{i}. {item['file']} {item['rule']} → {item['symbol']}: {item['message']} [{item['strategy']}]")
        command = input("[s]ave, [v]iew N, [e]dit N, [d]elete N, [b]an N, [r]evise N, [q]uit: ").strip().split()
        if not command:
            continue
        if command[0] == "s":
            return suggestions
        if command[0] == "q":
            return None
        if len(command) != 2 or not command[1].isdigit() or not 1 <= int(command[1]) <= len(suggestions):
            print("Select a suggestion number")
            continue
        index = int(command[1])-1
        item = suggestions[index]
        if command[0] == "v":
            site = by_id.get(item["id"])
            print(site["context"] if site else "Grammar site has changed or disappeared")
        elif command[0] == "d":
            suggestions.pop(index)
        elif command[0] == "b":
            cache.add(item["id"])
            suggestions.pop(index)
            (root / ".ispa-ai-cache.json").write_text(json.dumps({"banned": sorted(cache)}, indent=2)+"\n")
        elif command[0] == "e":
            item["message"] = input(f"Message [{item['message']}]: ").strip() or item["message"]
            strategy = input(f"Strategy [{item['strategy']}]: ").strip() or item["strategy"]
            if strategy in KINDS:
                item["strategy"] = strategy
                if strategy == "sync_tokens":
                    item["sync"] = [x.strip() for x in input("Sync token names (comma separated): ").split(",") if x.strip()]
            else:
                print("Invalid strategy; kept previous")
        elif command[0] == "r":
            if item["id"] not in by_id:
                print("Grammar site has changed; remove this suggestion first")
                continue
            if not config.get("ISPA_AI_PROVIDER"):
                config.update(configure(config_path()))
            instruction = input("How should the AI revise this suggestion? ").strip()
            revised = parse_answer(ask_ai(config, prompt_for([by_id[item["id"]]], instruction)), [by_id[item["id"]]])
            if revised:
                suggestions[index] = revised[0]
            else:
                print("AI returned no valid revision")


def run(args):
    cfg_path = config_path()
    if args.configure or not args.grammar and not args.review_only:
        config = configure(cfg_path)
    else:
        config = effective_config(read_env(cfg_path))
        print(f"Configuration: {cfg_path}; provider: {config.get('ISPA_AI_PROVIDER', '(none)')}; model: {config.get('ISPA_AI_MODEL', '(none)')}")
        if not config.get("ISPA_AI_PROVIDER") and not args.review_only:
            config = configure(cfg_path)
        elif not args.yes and not args.review_only and input("Configure provider? [y/N]: ").strip().lower() == "y":
            config = configure(cfg_path)
    grammar = args.grammar or input("Grammar directory or .isc entry: ").strip()
    if not grammar:
        raise ValueError("Grammar path is required")
    path = Path(grammar).expanduser().resolve()
    root = path if path.is_dir() else path.parent
    if not root.is_dir():
        raise ValueError(f"Grammar path does not exist: {path}")
    sites = scan(root)
    all_sites = sites
    cache = set(load_json(root / ".ispa-ai-cache.json", {"banned": []})["banned"])
    output_path = root / OUTPUT_FILE
    legacy_path = root / LEGACY_FILE
    existing = load_json(output_path if output_path.exists() else legacy_path,
                         {"version": 1, "suggestions": []})
    draft_path = root / ".ispa-ai-draft.json"
    draft = load_json(draft_path, {"version": 1, "suggestions": []})
    if getattr(args, "regenerate", False):
        if args.review_only:
            raise ValueError("--regenerate cannot be combined with --review-only")
        existing = {"version": 1, "suggestions": []}
        draft = {"version": 1, "suggestions": []}
    processed = set(draft.get("processed", []))
    current_sites = {site["id"]: site for site in all_sites}
    for item in existing.get("suggestions", []) + draft.get("suggestions", []):
        if item["id"] in current_sites:
            current = current_sites[item["id"]]
            for field in ("file", "rule", "symbol", "offset", "expected"):
                item[field] = current[field]
    draft_items = [item for item in draft.get("suggestions", []) if item["id"] not in cache]
    used = {item["id"] for item in existing.get("suggestions", []) + draft_items} | processed
    sites = [site for site in sites if site["id"] not in cache and site["id"] not in used]
    print(f"Found {len(sites)} unhandled grammar sites")
    if not sites and not existing.get("suggestions") and not draft_items:
        return 0
    suggestions = list(existing.get("suggestions", [])) + draft_items
    if not args.review_only:
        for start in range(0, len(sites), args.batch_size):
            chunk = sites[start:start+args.batch_size]
            print(f"Asking AI about sites {start+1}–{start+len(chunk)} of {len(sites)}")
            batch = suggest_chunk(config, chunk)
            suggestions.extend(batch)
            draft_items.extend(batch)
            processed.update(site["id"] for site in chunk)
            draft_path.write_text(json.dumps({"version": 1, "processed": sorted(processed),
                                              "suggestions": draft_items},
                                             ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    if not args.yes:
        suggestions = review(root, all_sites, suggestions, config, cache)
        if suggestions is None:
            return 0
    output = {"version": 1, "suggestions": suggestions}
    output_path.write_text(json.dumps(output, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    draft_path.unlink(missing_ok=True)
    print(f"Saved {len(suggestions)} suggestions to {output_path}")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description="Generate and review LL parser recovery suggestions")
    parser.add_argument("grammar", nargs="?", help="grammar directory or .isc entry file")
    parser.add_argument("--configure", action="store_true", help="configure provider first")
    parser.add_argument("--yes", action="store_true", help="save valid AI suggestions without interactive review")
    parser.add_argument("--review-only", action="store_true", help="review saved suggestions without contacting AI")
    parser.add_argument("--regenerate", action="store_true", help="ask AI to replace saved suggestions; keep current JSON until save")
    parser.add_argument("--models", action="store_true", help="list available Copilot model IDs")
    parser.add_argument("--batch-size", type=int, default=20)
    args = parser.parse_args(argv)
    try:
        if args.batch_size < 1:
            raise ValueError("Batch size must be positive")
        if args.models:
            config = effective_config(read_env(config_path()))
            if config.get("ISPA_AI_PROVIDER") != "copilot":
                raise ValueError("--models requires a Copilot configuration")
            print("\n".join(copilot_models(config)))
            return 0
        return run(args)
    except (ValueError, OSError, json.JSONDecodeError, KeyError, EOFError) as error:
        print(f"ispa-ai: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
