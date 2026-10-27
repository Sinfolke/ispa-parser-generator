#!/usr/bin/env sh
set -eu

REPO='Sinfolke/ispa-parser-generator'
INSTALL_ROOT="${ISPA_INSTALL_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/ispa}"
BIN_DIR="${ISPA_BIN_DIR:-$HOME/.local/bin}"
fail() { printf 'ispa-update: %s\n' "$*" >&2; exit 1; }
command -v curl >/dev/null 2>&1 || fail 'curl is required'
command -v python3 >/dev/null 2>&1 || fail 'python3 is required'

# The installer fetched below installs all release tools, including ispa-ai.
# The version marker is maintained by install.sh. Support older installations
# by recovering the tag from the existing symlink when possible.
if [ -f "$INSTALL_ROOT/current-version" ]; then
  current="$(head -n 1 "$INSTALL_ROOT/current-version")"
elif [ -L "$BIN_DIR/ispa" ]; then
  target="$(readlink "$BIN_DIR/ispa")"
  case "$target" in
    "$INSTALL_ROOT"/versions/*/ispa)
      current="$(basename "$(dirname "$target")")" ;;
    *) fail 'Cannot determine installed version; reinstall ISPA first' ;;
  esac
else
  fail 'ISPA installation not found; run install.sh first'
fi

scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT HUP INT TERM
curl -fsSL --retry 3 \
  -H 'Accept: application/vnd.github+json' \
  -H 'User-Agent: ISPA-updater' \
  "https://api.github.com/repos/$REPO/releases/latest" \
  -o "$scratch/latest.json" || fail 'Could not check GitHub Releases'

# Use semantic ordering (not lexical ordering). Refuse ambiguous version tags
# rather than risk replacing a newer version with an older one.
latest="$(python3 - "$scratch/latest.json" "$current" <<'PY'
import json, re, sys
with open(sys.argv[1], encoding='utf-8') as f:
    release = json.load(f)
latest = str(release.get('tag_name', ''))
current = sys.argv[2]
regex = re.compile(r'^v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-([0-9A-Za-z.-]+))?(?:\+[0-9A-Za-z.-]+)?$')
def parse(tag):
    m = regex.fullmatch(tag)
    if not m:
        raise SystemExit(f'Unsupported version tag {tag!r}; expected vMAJOR.MINOR.PATCH')
    return tuple(map(int, m.group(1, 2, 3))), m.group(4)
def prerelease_cmp(a, b):
    if a is None: return 0 if b is None else 1
    if b is None: return -1
    xs, ys = a.split('.'), b.split('.')
    for x, y in zip(xs, ys):
        xn, yn = x.isdigit(), y.isdigit()
        if xn and yn:
            diff = (int(x) > int(y)) - (int(x) < int(y))
        elif xn != yn:
            diff = -1 if xn else 1
        else:
            diff = (x > y) - (x < y)
        if diff: return diff
    return (len(xs) > len(ys)) - (len(xs) < len(ys))
a, apre = parse(current)
b, bpre = parse(latest)
newer = b > a or (b == a and prerelease_cmp(bpre, apre) > 0)
print(latest if newer else '')
PY
)" || fail 'Could not compare release versions'

if [ -z "$latest" ]; then
  printf 'ISPA %s is already up to date.\n' "$current"
  exit 0
fi
printf 'ISPA update available: %s -> %s\n' "$current" "$latest"
installer_url="https://raw.githubusercontent.com/$REPO/$latest/scripts/install.sh"
curl -fsSL --retry 3 "$installer_url" -o "$scratch/install.sh" || fail "Could not download installer for $latest"
ISPA_VERSION="$latest" ISPA_INSTALL_DIR="$INSTALL_ROOT" ISPA_BIN_DIR="$BIN_DIR" \
  sh "$scratch/install.sh"
printf 'Updated ISPA to %s.\n' "$latest"
