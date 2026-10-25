#!/usr/bin/env sh
set -eu

REPO="Sinfolke/ispa-parser-generator"
VERSION="${ISPA_VERSION:-latest}"
INSTALL_ROOT="${ISPA_INSTALL_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/ispa}"
BIN_DIR="${ISPA_BIN_DIR:-$HOME/.local/bin}"

fail() { printf 'ispa installer: %s\n' "$*" >&2; exit 1; }
command -v curl >/dev/null 2>&1 || fail 'curl is required'
command -v tar >/dev/null 2>&1 || fail 'tar is required'
command -v python3 >/dev/null 2>&1 || fail 'python3 is required for JSON release metadata and archive validation'

os="$(uname -s)"
arch="$(uname -m)"
case "$os:$arch" in
  Linux:x86_64|Linux:amd64) platform=ispa-linux-x64 ;;
  Darwin:arm64|Darwin:aarch64) platform=ispa-macos-arm64 ;;
  Darwin:x86_64) platform=ispa-macos-x64 ;;
  *) fail "Unsupported platform $os/$arch. Supported: Linux x86_64, macOS arm64, macOS x86_64 (if released)." ;;
esac

if [ "$VERSION" = latest ]; then
  api="https://api.github.com/repos/$REPO/releases/latest"
else
  api="https://api.github.com/repos/$REPO/releases/tags/$VERSION"
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
curl -fsSL --retry 3 -H 'Accept: application/vnd.github+json' "$api" -o "$tmp/release.json" || fail "Cannot fetch release metadata from $api"

# Read tag from GitHub's response; prevent accidental path traversal in directory names.
tag="$(python3 - "$tmp/release.json" <<'PY'
import json, re, sys
with open(sys.argv[1], encoding='utf-8') as f: data=json.load(f)
tag=data.get('tag_name', '')
if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._+-]*', tag):
    raise SystemExit('Invalid release tag in GitHub response')
print(tag)
PY
)" || fail 'Failed to read valid release tag'

archive="$platform.tar.gz"
url="https://github.com/$REPO/releases/download/$tag/$archive"
printf 'Installing ISPA %s (%s)\n' "$tag" "$platform"
curl -fL --retry 3 "$url" -o "$tmp/$archive" || fail "Release asset not available: $archive (release $tag may still be publishing)"

mkdir -p "$tmp/unpacked"
# Reject absolute paths, parent traversal, symlinks and hardlinks before extraction.
python3 - "$tmp/$archive" <<'PY'
import pathlib, sys, tarfile
with tarfile.open(sys.argv[1], 'r:gz') as t:
    for member in t.getmembers():
        p = pathlib.PurePosixPath(member.name)
        if p.is_absolute() or '..' in p.parts or not (member.isfile() or member.isdir()):
            raise SystemExit('Unsafe member in archive: ' + member.name)
PY

tar -xzf "$tmp/$archive" -C "$tmp/unpacked"
[ -f "$tmp/unpacked/ispa" ] || fail 'Archive does not contain ispa executable'
[ -f "$tmp/unpacked/ispa-pkg" ] || fail 'Archive does not contain ispa-pkg executable'
[ -f "$tmp/unpacked/ispadbg" ] || fail 'Archive does not contain ispadbg executable'
[ -f "$tmp/unpacked/install.sh" ] || fail 'Archive does not contain install.sh'
[ -f "$tmp/unpacked/update.sh" ] || fail 'Archive does not contain update.sh'
chmod 755 "$tmp/unpacked/ispa"
chmod 755 "$tmp/unpacked/ispa-pkg"
chmod 755 "$tmp/unpacked/ispadbg"

mkdir -p "$INSTALL_ROOT/versions" "$BIN_DIR"
target="$INSTALL_ROOT/versions/$tag"
# Install to a temporary sibling and atomically rename it on the same filesystem.
staging="$INSTALL_ROOT/versions/.install-$tag-$$"
trap 'rm -rf "$tmp" "$staging"' EXIT HUP INT TERM
mv "$tmp/unpacked" "$staging"
if [ -e "$target" ]; then
  if [ ! -f "$target/ispa-pkg" ]; then
    cp "$staging/ispa-pkg" "$target/ispa-pkg"
    chmod 755 "$target/ispa-pkg"
  fi
  if [ ! -f "$target/ispadbg" ]; then
    cp "$staging/ispadbg" "$target/ispadbg"
    chmod 755 "$target/ispadbg"
  fi
  rm -rf "$staging"
else
  mv "$staging" "$target"
fi
ln -sfn "$target/ispa" "$BIN_DIR/ispa"
ln -sfn "$target/ispa-pkg" "$BIN_DIR/ispa-pkg"
ln -sfn "$target/ispadbg" "$BIN_DIR/ispadbg"
cp "$target/install.sh" "$INSTALL_ROOT/install.sh"
cp "$target/update.sh" "$INSTALL_ROOT/update.sh"
printf '%s' "$tag" > "$INSTALL_ROOT/current-version"
cat > "$BIN_DIR/ispa-update" <<EOF
#!/usr/bin/env sh
exec sh "$INSTALL_ROOT/update.sh" "\$@"
EOF
chmod 755 "$BIN_DIR/ispa-update"

printf 'Installed %s\n' "$target"
printf 'Command: %s/ispa\n' "$BIN_DIR"
printf 'Package command: %s/ispa-pkg\n' "$BIN_DIR"
printf 'Debugger command: %s/ispadbg\n' "$BIN_DIR"
case ":$PATH:" in
  *":$BIN_DIR:"*) ;;
  *) printf 'Add this to your shell profile, then restart the shell:\n  export PATH="%s:$PATH"\n' "$BIN_DIR" ;;
esac
printf 'Try: "%s/ispa" --help\n' "$BIN_DIR"
