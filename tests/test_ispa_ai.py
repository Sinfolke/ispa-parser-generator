import io
import json
from pathlib import Path
import sys
import tempfile
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from urllib.error import HTTPError


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "cli/pkg"))
from ispa_pkg import ai


class AiTests(unittest.TestCase):
    def test_scan_skips_actions_and_existing_fail_marker(self):
        with tempfile.TemporaryDirectory() as temp:
            grammar = Path(temp)
            (grammar / "main.isc").write_text(
                "ID: [a-z]+;\nmain: @ ID %existing ';' @ {foo} ;\n"
                "fail existing { ctx.error(\"handled\") ctx.insert() }\n")
            sites = ai.scan(grammar)
            self.assertEqual([(site["rule"], site["symbol"]) for site in sites],
                             [("main", "ID")])
            self.assertEqual(sites[0]["offset"], len("ID: [a-z]+;\nmain: @ "))

    def test_scan_skips_directly_optional_symbols(self):
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp) / "main.isc").write_text("main: MAYBE? MANY* REQUIRED;\n")
            self.assertEqual([site["symbol"] for site in ai.scan(Path(temp))], ["REQUIRED"])

    def test_scan_ignores_nested_rule_heads_and_fail_action_code(self):
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp) / "main.isc").write_text(
                "main: @ ID | other;\n"
                "    #other:\n        @ NAME;\n"
                "fail handler {\n    if missing:\n        ctx.insert()\n    else:\n        ctx.replace()\n}\n"
            )
            sites = ai.scan(Path(temp))
            self.assertEqual([(site["rule"], site["symbol"]) for site in sites],
                             [("main", "ID"), ("other", "NAME")])

    def test_scan_ignores_python_code_in_advanced_rules_and_dotted_fail_blocks(self):
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp) / "main.isc").write_text(
                "TOKEN<part> {\n    match '{' part;\n"
                "    if ctx.next_char == '{':\n        ctx.skip()  # } is not the rule end\n"
                "    else:\n        ctx.error('missing }')\n}\n"
                "SMALL { match 'x'; ctx.skip() }\n"
                "main: TOKEN SMALL ID;\n"
                "fail main.missing {\n    if broken:\n        ctx.insert()\n"
                "    else:\n        ctx.abort()\n}\n"
            )
            self.assertEqual([(site["rule"], site["symbol"]) for site in ai.scan(Path(temp))],
                             [("main", "TOKEN"), ("main", "SMALL"), ("main", "ID")])

    def test_model_response_must_match_supplied_site_and_strategy(self):
        site = {"id": "a"*16, "file": "main.isc", "rule": "main", "symbol": "ID",
                "offset": 7, "expected": "ID"}
        answer = {"suggestions": [
            {"id": site["id"], "message": "Expected an identifier", "strategy": "insert", "sync": []},
            {"id": "b"*16, "message": "Unexpected", "strategy": "insert"},
            {"id": site["id"], "message": "Bad", "strategy": "python", "sync": []},
            {"id": site["id"], "message": "Implicit insert"},
        ]}
        self.assertEqual(len(ai.parse_answer(json.dumps(answer), [site])), 1)
        self.assertEqual(ai.parse_answer(json.dumps({"suggestions": [
            {"id": site["id"], "message": "Implicit insert"}]}), [site]), [])

    def test_lexer_rule_cannot_use_parser_only_recovery(self):
        site = {"id": "c"*16, "file": "main.isc", "rule": "STRING", "symbol": "'\\\\'",
                "offset": 8, "expected": "'\\\\'"}
        answer = {"suggestions": [{"id": site["id"], "message": "Bad escape",
                                    "strategy": "replace", "sync": []}]}
        self.assertEqual(ai.parse_answer(json.dumps(answer), [site]), [])

    def test_legacy_recovery_json_is_loaded_and_saved_with_json_extension(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "main.isc").write_text("main: ID;\n")
            site = ai.scan(root)[0]
            item = {key: site[key] for key in ("id", "file", "rule", "symbol", "offset", "expected")}
            item.update(message="Expected ID", strategy="insert", sync=[])
            (root / "ai.gle").write_text(json.dumps({"version": 1, "suggestions": [item]}))
            config = root / "config.env"
            args = SimpleNamespace(grammar=str(root), configure=False, review_only=True,
                                   yes=True, batch_size=20)
            with patch.object(ai, "config_path", return_value=config):
                self.assertEqual(ai.run(args), 0)
            self.assertEqual(json.loads((root / "ai.json").read_text())["suggestions"], [item])
            self.assertTrue((root / "ai.gle").exists())

    def test_regenerate_keeps_saved_json_when_provider_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "main.isc").write_text("main: ID;\n")
            saved = {"version": 1, "suggestions": [{"id": ai.scan(root)[0]["id"],
                                                       "message": "Old", "strategy": "insert"}]}
            output = root / "ai.json"
            output.write_text(json.dumps(saved))
            args = SimpleNamespace(grammar=str(root), configure=False, review_only=False,
                                   regenerate=True, yes=True, batch_size=20)
            with patch.object(ai, "config_path", return_value=root / "config.env"), \
                 patch.object(ai, "configure", return_value={"ISPA_AI_PROVIDER": "openai"}), \
                 patch.object(ai, "suggest_chunk", side_effect=ValueError("provider unavailable")) as mock:
                with self.assertRaisesRegex(ValueError, "provider unavailable"):
                    ai.run(args)
            self.assertEqual(mock.call_count, 1)
            self.assertEqual(json.loads(output.read_text()), saved)

    def test_ban_identity_survives_text_inserted_before_site(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "main.isc"
            path.write_text("main: ID ';';\n")
            before = next(site for site in ai.scan(Path(temp)) if site["symbol"] == "ID")
            path.write_text("// comment\nmain: ID ';';\n")
            after = next(site for site in ai.scan(Path(temp)) if site["symbol"] == "ID")
            self.assertEqual(before["id"], after["id"])
            self.assertNotEqual(before["offset"], after["offset"])

    def test_provider_request_uses_configured_endpoint(self):
        class Response(io.BytesIO):
            def __enter__(self):
                return self

            def __exit__(self, *_):
                self.close()

        body = {"choices": [{"message": {"content": '{"suggestions":[]}'}}]}
        with patch.object(ai, "urlopen", return_value=Response(json.dumps(body).encode())) as mock:
            text = ai.ask_ai({"ISPA_AI_PROVIDER": "openai", "ISPA_AI_URL": "https://example.test/chat/completions",
                              "ISPA_AI_MODEL": "example", "ISPA_AI_API_KEY": "test"}, "prompt")
        request = mock.call_args.args[0]
        self.assertEqual(request.full_url, "https://example.test/chat/completions")
        self.assertEqual(json.loads(request.data)["messages"][0]["content"], "prompt")
        self.assertEqual(request.get_header("User-agent"), "ispa-ai/0.1")
        self.assertEqual(text, '{"suggestions":[]}')

    def test_cloudflare_error_is_explained_without_echoing_key(self):
        key = "gsk_private_example"
        error = HTTPError("https://example.test", 403, "Forbidden", {},
                          io.BytesIO(f"error code: 1010; {key}".encode()))
        with patch.object(ai, "urlopen", side_effect=error):
            with self.assertRaisesRegex(ValueError, "Cloudflare Error 1010") as caught:
                ai.ask_ai({"ISPA_AI_PROVIDER": "openai", "ISPA_AI_URL": "https://example.test",
                           "ISPA_AI_MODEL": "example", "ISPA_AI_API_KEY": key}, "prompt")
        self.assertNotIn(key, str(caught.exception))

    def test_groq_uses_structured_output_and_completion_budget(self):
        class Response(io.BytesIO):
            def __enter__(self):
                return self

            def __exit__(self, *_):
                self.close()

        response = {"choices": [{"finish_reason": "stop", "message": {"content": '{"suggestions":[]}'}}]}
        with patch.object(ai, "urlopen", return_value=Response(json.dumps(response).encode())) as mock:
            ai.ask_ai({"ISPA_AI_PROVIDER": "openai",
                       "ISPA_AI_URL": "https://api.groq.com/openai/v1/chat/completions",
                       "ISPA_AI_MODEL": "openai/gpt-oss-20b", "ISPA_AI_API_KEY": "test"}, "prompt")
        body = json.loads(mock.call_args.args[0].data)
        self.assertEqual(body["max_completion_tokens"], 4096)
        self.assertEqual(body["reasoning_effort"], "low")
        self.assertTrue(body["response_format"]["json_schema"]["strict"])

    def test_legacy_token_url_uses_copilot_chat_headers(self):
        class Response(io.BytesIO):
            def __enter__(self):
                return self

            def __exit__(self, *_):
                self.close()

        response = {"choices": [{"finish_reason": "stop", "message": {"content": "OK"}}]}
        legacy = {"ISPA_AI_PROVIDER": "openai", "ISPA_AI_URL": ai.COPILOT_TOKEN_URL,
                  "ISPA_AI_MODEL": "GPT‑4.1", "ISPA_AI_API_KEY": "github-oauth"}
        with patch.object(ai, "copilot_token", return_value="copilot-short-token"), \
             patch.object(ai, "urlopen", return_value=Response(json.dumps(response).encode())) as mock:
            self.assertEqual(ai.ask_ai(legacy, "prompt"), "OK")
        request = mock.call_args.args[0]
        self.assertEqual(request.full_url, ai.COPILOT_CHAT_URL)
        self.assertEqual(request.get_header("Copilot-integration-id"), "vscode-chat")
        self.assertEqual(request.get_header("Authorization"), "Bearer copilot-short-token")
        self.assertNotIn("model", json.loads(request.data))

    def test_copilot_token_exchange_is_cached_until_expiry(self):
        class Response(io.BytesIO):
            def __enter__(self):
                return self

            def __exit__(self, *_):
                self.close()

        ai._copilot_cache.clear()
        payload = {"token": "short-token", "expires_at": time.time() + 600}
        with patch.object(ai, "urlopen", return_value=Response(json.dumps(payload).encode())) as mock:
            config = {"ISPA_AI_GITHUB_TOKEN": "github-oauth"}
            self.assertEqual(ai.copilot_token(config), "short-token")
            self.assertEqual(ai.copilot_token(config), "short-token")
        self.assertEqual(mock.call_count, 1)
        request = mock.call_args.args[0]
        self.assertEqual(request.full_url, ai.COPILOT_TOKEN_URL)
        self.assertEqual(request.get_header("Authorization"), "token github-oauth")
        ai._copilot_cache.clear()

    def test_copilot_device_login_waits_for_authorization(self):
        device = {"device_code": "private-device", "user_code": "ABCD-EFGH",
                  "verification_uri": "https://github.com/login/device", "interval": 1,
                  "expires_in": 900}
        with patch.object(ai, "request_json", side_effect=[device, {"error": "authorization_pending"},
                                                        {"access_token": "github-oauth"}]) as request, \
             patch.object(ai.time, "sleep") as sleep:
            self.assertEqual(ai.github_device_login(), "github-oauth")
        self.assertEqual(request.call_count, 3)
        self.assertEqual(sleep.call_count, 2)

    def test_local_copilot_proxy_needs_no_api_key(self):
        class Response(io.BytesIO):
            def __enter__(self):
                return self

            def __exit__(self, *_):
                self.close()

        response = {"choices": [{"finish_reason": "stop", "message": {"content": "OK"}}]}
        config = {"ISPA_AI_PROVIDER": "copilot-proxy",
                  "ISPA_AI_URL": "http://127.0.0.1:5000/v1/chat/completions",
                  "ISPA_AI_MODEL": ""}
        with patch.object(ai, "urlopen", return_value=Response(json.dumps(response).encode())) as mock:
            self.assertEqual(ai.ask_ai(config, "prompt"), "OK")
        request = mock.call_args.args[0]
        self.assertIsNone(request.get_header("Authorization"))
        self.assertNotIn("model", json.loads(request.data))

    def test_incomplete_batch_is_retried_in_smaller_groups(self):
        sites = [{"id": str(i), "file": "main.isc", "rule": "main", "symbol": "ID",
                  "offset": i, "expected": "ID", "kind": "token", "context": "main: ID;"}
                 for i in range(2)]
        def fake_answer(_, prompt):
            if prompt.count("main: ID;") > 1:
                raise ai.IncompleteResponse("truncated")
            site_id = "0" if '"id": "0"' in prompt else "1"
            return json.dumps({"suggestions": [{"id": site_id, "message": "Expected ID",
                                                "strategy": "insert", "sync": []}]})
        with patch.object(ai, "ask_ai", side_effect=fake_answer) as mock:
            result = ai.suggest_chunk({}, sites)
        self.assertEqual(len(result), 2)
        self.assertEqual(mock.call_count, 3)

    def test_interrupted_run_resumes_completed_batches(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "main.isc").write_text("main: ID ';';\n")
            config = root / "config.env"
            ai.save_env(config, {"ISPA_AI_PROVIDER": "openai", "ISPA_AI_URL": "https://example.test",
                                 "ISPA_AI_MODEL": "example", "ISPA_AI_API_KEY": "test"})
            args = SimpleNamespace(grammar=str(root), configure=False, review_only=False,
                                   yes=True, batch_size=1)
            def suggestion(_, sites):
                site = sites[0]
                return [{key: site[key] for key in ("id", "file", "rule", "symbol", "offset", "expected")}
                        | {"message": "Expected symbol", "strategy": "insert", "sync": []}]
            with patch.object(ai, "config_path", return_value=config), \
                 patch.object(ai, "suggest_chunk", side_effect=[suggestion(None, ai.scan(root)[:1]),
                                                                 ValueError("network down")]):
                with self.assertRaisesRegex(ValueError, "network down"):
                    ai.run(args)
            self.assertEqual(len(json.loads((root / ".ispa-ai-draft.json").read_text())["suggestions"]), 1)
            with patch.object(ai, "config_path", return_value=config), \
                 patch.object(ai, "suggest_chunk", side_effect=suggestion) as mock:
                self.assertEqual(ai.run(args), 0)
            self.assertEqual(mock.call_count, 1)
            self.assertEqual(len(json.loads((root / "ai.json").read_text())["suggestions"]), 2)
            self.assertFalse((root / ".ispa-ai-draft.json").exists())


if __name__ == "__main__":
    unittest.main()
