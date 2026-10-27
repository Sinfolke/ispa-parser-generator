# ispa-ai

`ispa-ai` suggests custom LL parser diagnostics and recovery actions. Install the
Python package from this repository (`pip install -e .`), then run:

```sh
ispa-ai --configure path/to/grammar
ispa -a LL --dir path/to/grammar path/to/grammar/entry.isc -o Parser
```

Without a grammar argument, the CLI shows its configuration and asks for the
grammar path. Configuration lives in `$XDG_DATA_HOME/ispa/.env`, or
`~/.local/share/ispa/.env` on Linux. The file is created with user-only
permissions. Choose `anthropic`, `openai` (also for any compatible endpoint),
`ollama`, `copilot`, or `copilot-proxy`, then supply the endpoint and model. The URL for an OpenAI compatible
provider must point to its `/chat/completions` endpoint. Ollama uses its local
OpenAI compatible endpoint by default.

For direct GitHub Copilot access, choose `copilot`. Enter a GitHub OAuth access
token from the [Copilot API guide](https://github.com/Alorse/copilot-to-api),
or leave the credential prompt empty to start GitHub device authorization.
`ispa-ai` exchanges that GitHub token for a short lived Copilot token, refreshes
it as needed, and sends chat requests to `api.githubcopilot.com` with the
`Copilot-Integration-Id` header. It keeps the exchanged token in memory. Run
`ispa-ai --models` to see exact model IDs; leave the model field empty to use
Copilot's default. An older configuration pointed at
`api.github.com/copilot_internal/v2/token` is recognized automatically.
The direct API in the linked guide is unofficial and may change.

If you run the guide's local server, choose `copilot-proxy` instead. Its default
URL targets the Python server on port 5000; enter port 3000 for the Node server.
The proxy holds the GitHub credential, so the CLI's proxy key is optional.

For Groq, choose `openai`, set the URL to
`https://api.groq.com/openai/v1/chat/completions`, and use a Groq model ID such
as `openai/gpt-oss-20b`. `ispa-ai` sends a descriptive user agent because some
edge services reject Python `urllib`'s default request signature.

The CLI scans `.isc` files, omits sites with an existing `%` fail marker, and
sends short grammar excerpts and symbol occurrences in batches. It accepts only
suggestions for the supplied site IDs. Review lets you edit a message or
strategy, remove a suggestion, permanently ban its site, or ask the model to
revise one suggestion. `ispa-ai --review-only path/to/grammar` opens saved
suggestions without an API call. Banned site IDs are kept in `.ispa-ai-cache.json` in the
grammar directory. It is ignored by Git so each user can keep their own choices.
Completed batches are saved in `.ispa-ai-draft.json`, so an interrupted run can
resume without requesting those sites again. Saving reviewed suggestions removes
the draft.

Saving creates `ai.json` at the grammar root with `"version": 1` and
an array of `suggestions`. Each suggestion contains its source path, byte offset,
exact symbol spelling, message, and strategy. The generator loads the file for
LL generation, checks the spelling at each offset, inserts a fail marker in
memory, and adds a corresponding `fail` block. It leaves `.isc` files untouched.
If an edit moves a site, rerun `ispa-ai` to refresh its offset. Generation
reports stale sites rather than applying them elsewhere. If a site disappears,
remove its suggestion during review.

Available strategies are `insert`, `replace`, and `sync_tokens`. `sync_tokens`
requires a list of lexer token names and inserts the missing symbol after
synchronizing. The generated fail block uses `ctx.error(...)` and the existing
LL recovery API. LR generation does not load `ai.json`. An existing `ai.gle`
JSON file is read during migration when `ai.json` is absent.

`ispa --ai-recovery path/to/ai.json` selects a recovery file explicitly.
`--no-ai-recovery` disables loading it for a run. Without either flag, ISPA
automatically looks for `ai.json` beside the entry grammar and its source
directory. The two flags cannot be combined.

Strategy selection is based on the grammar site, without a concrete malformed
input. The AI may reasonably choose `insert` often. The CLI requires an explicit
strategy in every new suggestion and skips directly optional symbols. Review
existing suggestions before using them; `replace` consumes one unexpected token,
and `sync_tokens` skips input until one of its named boundaries.
Lexer rule fail handlers support insertion but not replacement or token
synchronization. ISPA skips older suggestions that request those actions.

Use `--regenerate` to ask the provider for fresh suggestions for all eligible
sites. The existing `ai.json` stays in place until the new suggestions are saved.

The grammar scanner is conservative around actions and character classes, but
its site list is a candidate list. Review suggestions before saving, especially
for optional symbols, lexer declarations, and ambiguous alternatives.
Advanced rule bodies and `fail` blocks contain Python code and are excluded
from recovery site discovery.
