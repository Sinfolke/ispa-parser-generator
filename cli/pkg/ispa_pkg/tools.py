"""Install compiled ISPA tool release assets using the package archive checks."""
import hashlib
import json
import os
from pathlib import Path
import platform
import tempfile
from urllib.parse import quote


REPOSITORY = 'Sinfolke/ispa-parser-generator'
TOOLS = ('antlr-to-ispa',)


def release_platform():
    from .cli import PackageError
    system, machine = platform.system(), platform.machine().lower()
    arch = {'amd64': 'x64', 'x86_64': 'x64', 'arm64': 'arm64', 'aarch64': 'arm64'}.get(machine)
    target = {'Linux': 'linux', 'Windows': 'windows', 'Darwin': 'macos'}.get(system)
    result = f'{target}-{arch}'
    if result not in ('linux-x64', 'windows-x64', 'macos-x64', 'macos-arm64'):
        raise PackageError(f'No compiled tool release for {system}/{machine}')
    return result


def install_tool(name, root, version=None, destination=None, force=False):
    from .cli import PackageError, request, unpack
    if name not in TOOLS:
        raise PackageError(f'Unknown tool: {name}')
    target = release_platform()
    extension = '.zip' if target.startswith('windows') else '.tar.gz'
    executable = name + ('.exe' if target.startswith('windows') else '')
    archive_name = f'{name}-{target}{extension}'
    endpoint = 'latest' if version is None else 'tags/' + quote(version, safe='')
    release = json.loads(request(f'https://api.github.com/repos/{REPOSITORY}/releases/{endpoint}'))
    assets = {asset['name']: asset for asset in release.get('assets', [])}
    if archive_name not in assets:
        raise PackageError(f'Release {release.get("tag_name", version)} has no {archive_name}')
    asset = assets[archive_name]
    data = request(asset['browser_download_url'])
    digest = asset.get('digest')
    if digest and digest.startswith('sha256:') and hashlib.sha256(data).hexdigest() != digest[7:]:
        raise PackageError(f'Checksum mismatch for {archive_name}')
    destination = Path(destination) if destination else root.parent / 'bin'
    destination.mkdir(parents=True, exist_ok=True)
    installed = destination / executable
    if installed.exists() and not force:
        raise PackageError(f'{installed} already exists; use --force to replace it')
    with tempfile.TemporaryDirectory(prefix='.ispa-tool-', dir=destination) as temporary:
        content = unpack(data, archive_name, Path(temporary) / 'archive')
        binary = content / executable
        if not binary.is_file():
            raise PackageError(f'Archive does not contain {executable}')
        binary.chmod(0o755)
        os.replace(binary, installed)
    print(f'Installed {name} {release.get("tag_name", "")} to {installed}')
    print(f'Add {destination} to PATH if it is not already present.')
    return installed
