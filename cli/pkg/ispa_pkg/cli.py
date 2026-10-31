"""Download and install ISPA libraries from GitHub, archives, or a YAML catalog."""

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import tempfile
from urllib.parse import quote, urlparse
from urllib.request import Request, urlopen
import zipfile

import yaml


CATALOG_URL = "https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/ispa-libraries.yaml"
DEFAULT_CATALOG = {"packages": {
    "std": {"github": "Sinfolke/ispa-parser-generator", "ref": "libraries-{generator}-latest"},
    "target-core": {"github": "Sinfolke/ispa-parser-generator", "ref": "libraries-{generator}-latest",
                    "manifest": "ispa-package.target-core.yaml"},
    "target-cpp": {"github": "Sinfolke/ispa-parser-generator", "ref": "libraries-{generator}-latest",
                   "manifest": "ispa-package.target-cpp.yaml"},
    "python-target": {"github": "Sinfolke/ispa-parser-generator", "ref": "libraries-{generator}-latest",
                      "manifest": "ispa-package.python-target.yaml"},
    "rust-target": {"github": "Sinfolke/ispa-parser-generator", "ref": "libraries-{generator}-latest",
                    "manifest": "ispa-package.rust-target.yaml"},
}}
FOLDERS = ("ispalib", "ispatarget", "ispaalgorithm")
GITHUB = re.compile(r"^[\w.-]+/[\w.-]+$")
NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")


def default_root():
    if os.environ.get("ISPA_PKG_ROOT"):
        return Path(os.environ["ISPA_PKG_ROOT"])
    if os.environ.get("ISPA_INSTALL_DIR"):
        return Path(os.environ["ISPA_INSTALL_DIR"]) / "libraries"
    # Nuitka onefile keeps the original executable location in argv[0].
    for executable_name in (sys.argv[0], sys.executable):
        executable = Path(executable_name).resolve()
        if executable.parent.parent.name == "versions":
            return executable.parent.parent.parent / "libraries"
    if os.name == "nt" and os.environ.get("LOCALAPPDATA"):
        return Path(os.environ["LOCALAPPDATA"]) / "Programs" / "ISPA" / "libraries"
    if os.environ.get("XDG_DATA_HOME"):
        return Path(os.environ["XDG_DATA_HOME"]) / "ispa" / "libraries"
    return Path.home() / ".local" / "share" / "ispa" / "libraries"


class PackageError(Exception):
    pass


def python_command():
    """Use a real interpreter for helpers, including in frozen CLI releases."""
    # Nuitka does not reliably expose __compiled__ here, and a onefile build's sys.executable points at a
    # non-existent <tmpdir>/python, so validate the executable itself instead of trusting a frozen flag.
    interpreter = sys.executable
    if (interpreter and not globals().get('__compiled__') and not getattr(sys, 'frozen', False)
            and Path(interpreter).name.lower().startswith('python') and Path(interpreter).is_file()):
        return [interpreter]
    for name in ('python3', 'python'):
        executable = shutil.which(name)
        if executable:
            return [executable]
    if os.name == 'nt':
        launcher = shutil.which('py')
        if launcher:
            return [launcher, '-3']
    raise PackageError('Python 3 is required to build this package; install Python 3 and add it to PATH.')


class Status:
    """A single progress line for install/build steps.

    On a terminal it overwrites itself in place; piped output (or --verbose) gets one line per step instead,
    since an overwritten line is useless in a log."""

    def __init__(self):
        self.verbose = False
        self.tty = sys.stderr.isatty()
        self._width = 0

    def update(self, message):
        if self.verbose or not self.tty:
            print(message, file=sys.stderr)
            return
        sys.stderr.write('\r' + message + ' ' * max(0, self._width - len(message)))
        sys.stderr.flush()
        self._width = len(message)

    def done(self):
        if self.tty and not self.verbose and self._width:
            sys.stderr.write('\r' + ' ' * self._width + '\r')
            sys.stderr.flush()
            self._width = 0


STATUS = Status()


def run_build_step(command, cwd=None):
    """Run a noisy build command (cmake, cargo): streamed under --verbose, captured and shown only on failure otherwise."""
    if STATUS.verbose:
        subprocess.run(command, check=True, cwd=cwd, stdout=sys.stderr)
        return
    output = []
    with subprocess.Popen(command, cwd=cwd, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True) as process:
        for line in process.stdout:
            output.append(line)
            if STATUS.tty:
                STATUS.update(line.strip())
        code = process.wait()
    if code != 0:
        STATUS.done()
        sys.stderr.write(''.join(output))
        raise subprocess.CalledProcessError(code, command)


def read_yaml(data, label):
    try:
        parsed = yaml.safe_load(data) or {}
    except yaml.YAMLError as error:
        raise PackageError(f"Invalid YAML in {label}: {error}") from error
    if not isinstance(parsed, dict):
        raise PackageError(f"{label} must contain a YAML mapping")
    return parsed


def safe_relative(value):
    if not isinstance(value, str) or not value or "\\" in value:
        raise PackageError(f"Invalid relative path: {value!r}")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ("..", ".") for part in value.split("/")) or ":" in path.parts[0]:
        raise PackageError(f"Unsafe relative path: {value!r}")
    return Path(*path.parts)


def request(url):
    try:
        with urlopen(Request(url, headers={"User-Agent": "ispa-pkg", "Accept": "application/vnd.github+json"}), timeout=30) as response:
            return response.read()
    except Exception as error:
        raise PackageError(f"Cannot download {url}: {error}") from error


def fetch_file(location, base=None):
    if location.startswith(("https://", "http://")):
        return request(location)
    path = Path(location)
    if not path.is_absolute() and base is not None:
        path = base / safe_relative(location)
    return path.read_bytes()


def unpack(data, name, destination):
    destination.mkdir(parents=True, exist_ok=True)
    archive = destination / "archive"
    archive.write_bytes(data)
    destination = destination / "content"
    destination.mkdir()

    def valid(name):
        while name.startswith("./"):
            name = name[2:]
        if name.endswith("/"):
            name = name[:-1]
        if not name or name == ".":
            return None
        return safe_relative(name)

    if name.endswith(".zip"):
        with zipfile.ZipFile(archive) as src:
            for entry in src.infolist():
                relative = valid(entry.filename)
                if relative is None:
                    continue
                target = destination / relative
                mode = entry.external_attr >> 16
                if (mode & 0o170000) == 0o120000:
                    raise PackageError(f"Symlink in archive: {entry.filename}")
                if entry.is_dir():
                    target.mkdir(parents=True, exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(src.read(entry))
    elif name.endswith((".tar.gz", ".tgz")):
        with tarfile.open(archive, "r:gz") as src:
            for entry in src:
                relative = valid(entry.name)
                if relative is None:
                    continue
                target = destination / relative
                if entry.isdir():
                    target.mkdir(parents=True, exist_ok=True)
                elif entry.isfile():
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with src.extractfile(entry) as stream, target.open("wb") as output:
                        shutil.copyfileobj(stream, output)
                else:
                    raise PackageError(f"Unsupported archive member: {entry.name}")
    else:
        raise PackageError("Archive must end in .zip, .tar.gz, or .tgz")
    children = list(destination.iterdir())
    return children[0] if len(children) == 1 and children[0].is_dir() else destination


def github_source(repo, ref, workspace):
    if not GITHUB.fullmatch(repo):
        raise PackageError(f"Invalid GitHub repository: {repo}")
    sha = json.loads(request(f"https://api.github.com/repos/{repo}/commits/{quote(ref, safe='')}"))["sha"]
    data = request(f"https://api.github.com/repos/{repo}/tarball/{sha}")
    return unpack(data, "source.tar.gz", workspace), sha


def source_tree(spec, workspace):
    if "github" in spec:
        return github_source(spec["github"], spec.get("ref", "main"), workspace)
    if "archive" in spec:
        location = spec["archive"]
        data = fetch_file(location)
        return unpack(data, urlparse(location).path, workspace), hashlib.sha256(data).hexdigest()
    if "path" in spec:
        path = Path(spec["path"]).expanduser().resolve()
        if not path.is_dir():
            raise PackageError(f"Directory not found: {path}")
        return path, "local"
    raise PackageError("Package source must have github, archive, or path")


def generator_version(required=False):
    """Read the generator actually used for package builds, including ISPA overrides."""
    executable = os.environ.get("ISPA") or shutil.which("ispa")
    if executable:
        try:
            result = subprocess.run([executable, "--version"], check=True,
                                    capture_output=True, text=True, timeout=30)
            version = result.stdout.splitlines()[0].strip().rstrip(";").strip()
            if NAME.fullmatch(version):
                return version
        except (OSError, subprocess.SubprocessError, IndexError):
            pass
    if required:
        raise PackageError("Cannot determine generator version; set ISPA or put ispa on PATH")
    return "unknown"


def resolved_source(spec, version):
    spec = dict(spec)
    if "github" in spec and "{generator}" in spec.get("ref", ""):
        if version == "unknown":
            raise PackageError("A generator-specific package requires ispa; set ISPA or put ispa on PATH")
        # Development generators consume the development libraries.
        spec["ref"] = "main" if version == "dev" else spec["ref"].replace("{generator}", version)
    return spec


def installed_source(name, data, root, catalog_path):
    if data.get("from_catalog"):
        return catalog(root, catalog_path).get("packages", {}).get(name, data["source"])
    return data["source"]


def layout_entries(config):
    layout = config.get("layout", {})
    if not isinstance(layout, dict):
        raise PackageError("layout must be a mapping")
    result = []
    for folder in FOLDERS:
        mapping = layout.get(folder)
        if mapping is None:
            defaults = {"ispalib": ("grammar", "python"), "ispatarget": (), "ispaalgorithm": ()}
            mapping = {name: f"{folder}/{name}" for name in defaults[folder]}
            if folder == "ispatarget":
                mapping = {"cpp": "ispatarget/cpp", "python": "ispatarget/python",
                           "rust": "ispatarget/rust"}
            if folder == "ispaalgorithm":
                mapping = {".": "ispaalgorithm"}
        if not isinstance(mapping, dict):
            raise PackageError(f"layout.{folder} must be a mapping")
        for name, source in mapping.items():
            if not isinstance(name, str) or not isinstance(source, str):
                raise PackageError(f"Invalid layout.{folder} entry")
            dest = Path(folder) if name == "." else Path(folder) / safe_relative(name)
            result.append((safe_relative(source), dest))
    files = layout.get("files", {})
    if not isinstance(files, dict):
        raise PackageError("layout.files must be a mapping")
    file_entries = [(safe_relative(source), safe_relative(dest)) for dest, source in files.items()]
    root = safe_relative(layout.get("root", ".")) if layout.get("root", ".") != "." else Path(".")
    return root, result, file_entries


def build_cpp_library(package, name, root, required=False, kind='cpp', standard=17, output=None):
    """Compile a package's C++ sources into a static archive beside its headers.

    `kind` is the ispatarget directory to build; `output` names the archive (default: the package name)."""
    cpp = package / 'ispatarget' / kind
    if kind == 'target-cpp':
        generator = os.environ.get('ISPA') or shutil.which('ispa')
        library = root / 'target-core' / 'ispalib' / 'python'
        if generator and (cpp / 'regen.py').is_file() and library.is_dir():
            STATUS.update(f'{name}: generating C++ runtime from Python ispa_std')
            run_build_step(python_command() + [str(cpp / 'regen.py'), '--ispa', generator,
                            '--library', str(library)])
        elif not (cpp / 'ispa_core_lowered.h').is_file():
            raise PackageError(f'{name}: generating the runtime requires ispa and target-core')
    output = output or name
    sources = sorted(source for source in cpp.rglob('*.cpp') if source.is_file())
    if not sources:
        return None
    cmake = shutil.which('cmake')
    if not cmake:
        if required:
            raise PackageError('Building C++ libraries requires CMake')
        STATUS.update(f'{name}: skipping C++ build (CMake not found)')
        return None
    compiler = shlex.split(os.environ.get('CXX', ''))
    includes = [cpp, cpp / 'include']
    if kind != 'cpp':
        # The native parts of the package itself are not installed yet while it is staged.
        sibling = package / 'ispatarget' / 'cpp'
        includes.extend((sibling, sibling / 'include'))
    for other in discover_exports(root).get('cpp', []):
        includes.extend((other, other / 'include'))
    with tempfile.TemporaryDirectory(prefix='.ispa-cpp-', dir=root) as temporary:
        work = Path(temporary)
        def cmake_quote(value):
            return '"' + str(value).replace('\\', '\\\\').replace('"', '\\"') + '"'

        lines = ['cmake_minimum_required(VERSION 3.15)',
                 'project(ispa_package LANGUAGES CXX)',
                 'add_library(ispa_package STATIC']
        lines.extend(f'  {cmake_quote(source)}' for source in sources)
        lines.extend([')', 'set_target_properties(ispa_package PROPERTIES',
                      f'  OUTPUT_NAME {cmake_quote(output)}',
                      f'  ARCHIVE_OUTPUT_DIRECTORY {cmake_quote(work)}', ')',
                      f'target_compile_features(ispa_package PRIVATE cxx_std_{standard})'])
        valid_includes = [directory for directory in dict.fromkeys(includes) if directory.is_dir()]
        if valid_includes:
            lines.append('target_include_directories(ispa_package PRIVATE')
            lines.extend(f'  {cmake_quote(directory)}' for directory in valid_includes)
            lines.append(')')
        (work / 'CMakeLists.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')
        configure = [cmake, '-S', str(work), '-B', str(work / 'build')]
        flags = []
        if compiler:
            configure.extend(['-DCMAKE_CXX_COMPILER=' + compiler[0]])
            flags.extend(compiler[1:])
        if os.environ.get('CXXFLAGS'):
            flags.extend(shlex.split(os.environ['CXXFLAGS']))
        if flags:
            configure.append('-DCMAKE_CXX_FLAGS=' + ' '.join(shlex.quote(arg) for arg in flags))
        STATUS.update(f'{name}: configuring CMake C++ library')
        try:
            run_build_step(configure)
        except subprocess.CalledProcessError as error:
            if required:
                raise PackageError(f'{name}: CMake could not configure a C++ toolchain') from error
            STATUS.update(f'{name}: skipping C++ build (CMake could not configure a C++ toolchain)')
            return None
        STATUS.update(f'{name}: building C++ library')
        run_build_step([cmake, '--build', str(work / 'build'), '--config', 'Release', '--parallel'])
        archive_names = (f'{output}.lib', f'lib{output}.a')
        matches = [candidate for search_root in (work, work / 'build')
                   for archive_name in archive_names for candidate in search_root.rglob(archive_name)]
        if not matches:
            raise PackageError(f'CMake did not produce a static library for {output}')
        archive = matches[0]
        destination = cpp / 'lib' / archive.name
        destination.parent.mkdir(parents=True, exist_ok=True)
        archive.replace(destination)
    return destination


def build_missing_cpp_library(package, name, root):
    cpp = package / 'ispatarget' / 'cpp'
    if not any(cpp.rglob('*.cpp')):
        return build_cpp_core(package, name, root, only_missing=True)
    library_dir = cpp / 'lib'
    if not (library_dir / f'lib{name}.a').is_file() and not (library_dir / f'{name}.lib').is_file():
        return build_cpp_library(package, name, root)
    return build_cpp_core(package, name, root, only_missing=True)


def regenerate_cpp_core_deps(core, name):
    """Refresh ispastdlib_deps.json/.inc and ispaext_deps.inc from the installed headers.

    They are a scan of ispastdlib/ (tools/ispa_deps.py), not source, so ispa-pkg regenerates them
    on every install instead of trusting whatever the package happened to ship."""
    deps_script = core / 'tools' / 'ispa_deps.py'
    if not deps_script.is_file():
        return
    try:
        subprocess.run(python_command() + [str(deps_script), '--root', str(core),
                         '--out', str(core / 'ispastdlib_deps.json'),
                         '--emit-inc', str(core / 'ispastdlib_deps.inc')],
                       check=True, cwd=core)
        ext_script = core / 'tools' / 'gen_ext_deps.py'
        if ext_script.is_file():
            if not (core / 'ispaext_deps.json').is_file():
                (core / 'ispaext_deps.json').write_text('{}')
            subprocess.run(python_command() + [str(ext_script)], check=True, cwd=core)
    except subprocess.CalledProcessError as error:
        STATUS.update(f'{name}: skipping dependency manifest regeneration ({error})')


def build_cpp_core(package, name, root, only_missing=False):
    """Build the Python-derived C++ runtime and its native interoperability layer."""
    core = package / 'ispatarget' / 'target-cpp'
    if not core.is_dir() or not any(core.rglob('*.cpp')):
        return None
    regenerate_cpp_core_deps(core, name)
    output = 'ispa-core'
    library_dir = core / 'lib'
    if only_missing and ((library_dir / f'lib{output}.a').is_file() or (library_dir / f'{output}.lib').is_file()):
        return None
    return build_cpp_library(package, name, root, required=True,
                             kind='target-cpp', standard=17, output=output)


def ensure_requirements(name, config, root, allow_scripts, catalog_path, active):
    """Install the packages `requires:` names that are not installed yet, from the catalog."""
    requires = config.get("requires", [])
    if isinstance(requires, str):
        requires = [requires]
    if not isinstance(requires, list) or any(not isinstance(item, str) or not NAME.fullmatch(item) for item in requires):
        raise PackageError("requires must be a list of package names")
    for dependency in requires:
        if dependency == name or dependency in active:
            raise PackageError(f"{name}: circular requires on {dependency}")
        marker = root / dependency / ".ispa-package.json"
        if marker.is_file():
            data = json.loads(marker.read_text())
            if data.get("generator_version") == generator_version():
                continue
            install(dependency, installed_source(dependency, data, root, catalog_path), root,
                    allow_scripts, from_catalog=data.get("from_catalog", False),
                    catalog_path=catalog_path, _active=(*active, name))
            continue
        packages = catalog(root, catalog_path).get("packages", {})
        spec = packages.get(dependency) if isinstance(packages, dict) else None
        if not isinstance(spec, dict):
            raise PackageError(f"{name} requires {dependency}, which is not installed and not in the catalog")
        STATUS.update(f"{name}: installing required package {dependency}")
        install(dependency, dict(spec), root, allow_scripts, from_catalog=True,
                catalog_path=catalog_path, _active=(*active, name))


def build_rust_package(package, name, required=False):
    """Build a package's Rust target crate in release mode."""
    rust = package / 'ispatarget' / 'rust'
    manifests = sorted(rust.rglob('Cargo.toml')) if rust.is_dir() else []
    if not manifests:
        return None
    cargo = shutil.which('cargo')
    if not cargo:
        if required:
            raise PackageError('Building Rust packages requires Cargo (install the Rust toolchain)')
        STATUS.update(f'{name}: skipping Rust build (Cargo not found)')
        return None
    for manifest in manifests:
        STATUS.update(f'{name}: building Rust crate {manifest.parent.relative_to(rust)}')
        try:
            run_build_step([cargo, 'build', '--release', '--manifest-path', str(manifest)], cwd=manifest.parent)
        except subprocess.CalledProcessError as error:
            if required:
                raise PackageError(f'{name}: Cargo could not build {manifest.parent.name}') from error
            raise
    return manifests


def build_missing_rust_package(package, name):
    rust = package / 'ispatarget' / 'rust'
    if rust.is_dir() and any(rust.rglob('Cargo.toml')):
        return build_rust_package(package, name)
    return None


def install(name, spec, root, allow_scripts=False, force=False, from_catalog=False, catalog_path=None, _active=()):
    if not NAME.fullmatch(name):
        raise PackageError(f"Invalid package name: {name}")
    version = generator_version()
    requested_spec = dict(spec)
    spec = resolved_source(spec, version)
    root.mkdir(parents=True, exist_ok=True)
    target = root / name
    with tempfile.TemporaryDirectory(prefix=".ispa-pkg-", dir=root) as temp:
        work = Path(temp)
        tree, revision = source_tree(spec, work)
        config_tree = tree
        config_file = tree / safe_relative(spec.get("manifest", "ispa-package.yaml"))
        if "manifest" in spec and not config_file.is_file():
            raise PackageError(f"Package manifest not found: {spec['manifest']}")
        config = read_yaml(config_file.read_text(encoding="utf-8"), str(config_file)) if config_file.is_file() else {}
        ensure_requirements(name, config, root, allow_scripts, catalog_path, _active)
        if "archive" in config:
            location = config["archive"]
            data = fetch_file(location, tree)
            nested = work / "nested"
            nested.mkdir()
            tree = unpack(data, urlparse(location).path, nested)
            revision += ":" + hashlib.sha256(data).hexdigest()
        root_path, entries, file_entries = layout_entries(config)
        source_root = tree / root_path
        if not source_root.is_dir():
            raise PackageError(f"Layout root not found: {root_path}")
        scripts = config.get("install", [])
        if isinstance(scripts, str):
            scripts = [scripts]
        if not isinstance(scripts, list) or any(not isinstance(item, str) for item in scripts):
            raise PackageError("install must be a list of script paths")
        if "path" in spec:
            digest = hashlib.sha256()
            inputs = [source_root / src for src, _ in entries] + [source_root / src for src, _ in file_entries]
            inputs += [config_tree / safe_relative(script) for script in scripts]
            for origin in inputs:
                if origin.is_symlink():
                    raise PackageError(f"Symlink in library: {origin}")
                if origin.is_file():
                    files = [origin]
                elif origin.is_dir():
                    members = list(origin.rglob("*"))
                    for member in members:
                        if member.is_symlink():
                            raise PackageError(f"Symlink in library: {member}")
                    files = sorted(file for file in members if file.is_file())
                else:
                    continue
                for file in files:
                    if file.is_symlink():
                        raise PackageError(f"Symlink in library: {file}")
                    try:
                        relative = file.relative_to(source_root)
                        digest.update(b"library/")
                    except ValueError:
                        relative = file.relative_to(config_tree)
                        digest.update(b"repository/")
                    digest.update(relative.as_posix().encode())
                    digest.update(file.read_bytes())
            revision = digest.hexdigest()
        fingerprint = hashlib.sha256(json.dumps({"spec": spec, "revision": revision, "config": config,
                                                "from_catalog": from_catalog, "generator_version": version}, sort_keys=True).encode()).hexdigest()
        marker = target / ".ispa-package.json"
        if marker.is_file() and not force:
            try:
                if json.loads(marker.read_text())["fingerprint"] == fingerprint:
                    if build_missing_cpp_library(target, name, root):
                        refresh_saved_exports(root)
                    STATUS.done()
                    print(f"{name}: up to date ({revision[:12]})")
                    return
            except (ValueError, KeyError):
                pass
        if scripts and not allow_scripts:
            raise PackageError(f"{name} declares install scripts; rerun with --allow-scripts to execute them")
        stage = work / "stage"
        stage.mkdir()
        copied = 0
        for src, dest in entries:
            origin = source_root / src
            if origin.is_symlink():
                raise PackageError(f"Symlink in library: {origin}")
            if not origin.is_dir():
                continue
            destination = stage / dest
            shutil.copytree(origin, destination, symlinks=False, dirs_exist_ok=True)
            copied += 1
        for src, dest in file_entries:
            origin = source_root / src
            if not origin.is_file() or origin.is_symlink():
                raise PackageError(f"Layout file not found: {src}")
            destination = stage / dest
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(origin, destination)
            copied += 1
        if not copied:
            raise PackageError(f"No library folders found in {source_root}")
        for script in scripts:
            script_path = config_tree / safe_relative(script)
            if not script_path.is_file() or script_path.is_symlink():
                raise PackageError(f"Install script not found: {script}")
            if script_path.suffix == ".py":
                command = python_command() + [str(script_path), str(stage)]
            elif os.name == "nt":
                raise PackageError("Shell install scripts require a POSIX shell")
            else:
                command = ["sh", str(script_path), str(stage)]
            subprocess.run(command, cwd=config_tree, check=True)
        build_cpp_library(stage, name, root)
        build_cpp_core(stage, name, root)
        build_rust_package(stage, name)
        marker_data = {"name": name, "source": requested_spec, "resolved_source": spec,
                       "revision": revision, "fingerprint": fingerprint,
                       "generator_version": version, "from_catalog": from_catalog}
        (stage / ".ispa-package.json").write_text(json.dumps(marker_data, indent=2) + "\n")
        backup = work / "previous"
        if target.exists():
            target.rename(backup)
        try:
            stage.rename(target)
        except Exception:
            if backup.exists():
                backup.rename(target)
            raise
        refresh_saved_exports(root)
        STATUS.done()
        print(f"{name}: installed ({revision[:12]}) at {target}")


def catalog(root, path=None):
    custom = path is not None
    path = Path(path) if custom else root / "catalog.yaml"
    data = read_yaml(path.read_text(encoding="utf-8"), str(path)) if path.is_file() else DEFAULT_CATALOG
    if not custom and isinstance(data.get("packages"), dict):
        # Upgrade cached copies of the old official catalog. Explicit custom catalogs
        # and refs other than the old default retain their requested source.
        for name, spec in data["packages"].items():
            default = DEFAULT_CATALOG["packages"].get(name)
            if (default and isinstance(spec, dict) and spec.get("ref") == "main"
                    and spec.get("github") == default["github"]
                    and spec.get("manifest") == default.get("manifest")):
                spec["ref"] = default["ref"]
    return data


def parse_source(value):
    if value.startswith("https://github.com/"):
        value = value.removeprefix("https://github.com/").removesuffix(".git").rstrip("/")
    if Path(value).is_dir():
        return {"path": str(Path(value).resolve())}
    if GITHUB.fullmatch(value):
        return {"github": value, "ref": "main"}
    if value.endswith((".zip", ".tar.gz", ".tgz")):
        return {"archive": value}
    raise PackageError(f"Unknown package or source: {value}")



def discover_exports(root):
    """Discover only installed, marked packages; avoid temporary/untrusted folders."""
    targets = {}
    for marker in sorted(root.glob("*/.ispa-package.json")):
        package = marker.parent
        target_root = package / "ispatarget"
        if not target_root.is_dir():
            continue
        for target in sorted(target_root.iterdir()):
            if target.is_dir() and not target.is_symlink():
                targets.setdefault(target.name, []).append(target.resolve())
    return targets


def export_data(root):
    targets = discover_exports(root)
    cpp_dirs = targets.get("cpp", [])
    # Include the target root: consumers may include headers by their relative path.
    # Also include conventional include/ directories if present.
    core_dirs = targets.get("target-cpp", [])
    includes = list(dict.fromkeys(
        directory for target in cpp_dirs + core_dirs
        for directory in (target, target / "include")
        if directory.is_dir()
    ))
    sources = sorted({source.resolve() for target in cpp_dirs
                      for source in target.rglob("*.cpp") if source.is_file()})
    libraries = sorted({file.resolve() for target in cpp_dirs + core_dirs for file in target.rglob('*')
                        if file.is_file() and (file.suffix in ('.a', '.so', '.dylib', '.lib')
                                               or '.so.' in file.name)})
    library_dirs = sorted({file.parent for file in libraries})
    runtime_dirs = sorted({file.parent for target in cpp_dirs + core_dirs for file in target.rglob('*')
                           if file.is_file() and (file.suffix in ('.so', '.dylib', '.dll')
                                                  or '.so.' in file.name)})
    python_roots = sorted({target.parent for target in targets.get('python', [])})
    python_roots += sorted({marker.parent / 'ispatarget' for marker in root.glob('*/.ispa-package.json')
                            if (marker.parent / 'ispatarget').is_dir()
                            and any((marker.parent / 'ispatarget').glob('*.py'))})
    python_dirs = list(dict.fromkeys(python_roots + targets.get('python', [])))
    return targets, includes, sources, libraries, library_dirs, runtime_dirs, python_dirs


def cmake_quote(path):
    return '"' + str(path).replace('\\', '/').replace('"', '\\"').replace(';', '\\;') + '"'


def write_export_files(root):
    """Write a CMake import and JSON manifest, refreshing them on every export."""
    root.mkdir(parents=True, exist_ok=True)
    # Older installations may contain sources from before ispa-pkg built archives.
    for marker in sorted(root.glob('*/.ispa-package.json')):
        package = marker.parent
        build_missing_cpp_library(package, package.name, root)
        build_missing_rust_package(package, package.name)
    targets, includes, sources, libraries, library_dirs, runtime_dirs, python_dirs = export_data(root)
    output = root / '.ispa-exports'
    output.mkdir(parents=True, exist_ok=True)
    manifest = {
        'targets': {name: [str(path) for path in paths] for name, paths in sorted(targets.items())},
        'cpp': {'include_dirs': [str(path) for path in includes],
                'sources': [str(path) for path in sources],
                'library_dirs': [str(path) for path in library_dirs],
                'runtime_dirs': [str(path) for path in runtime_dirs],
                'libraries': [str(path) for path in libraries],
                # the `--stdlib core` archives (ispatarget/target-cpp/lib), also part of `libraries`
                'core_libraries': [str(path) for path in libraries if 'target-cpp' in path.parts]},
        'python': {'interpreter_dirs': [str(path) for path in python_dirs]},
    }
    (output / 'targets.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    lines = [
        '# Generated by ispa-pkg --export. Do not edit.',
        'if(NOT TARGET ispa_targets)',
        '  add_library(ispa_targets INTERFACE)',
        '  add_library(ISPA::Targets ALIAS ispa_targets)',
    ]
    if includes:
        lines += ['  target_include_directories(ispa_targets INTERFACE',
                  *('    ' + cmake_quote(path) for path in includes), '  )']
    if library_dirs:
        lines += ['  target_link_directories(ispa_targets INTERFACE',
                  *('    ' + cmake_quote(path) for path in library_dirs), '  )']
    if libraries:
        lines += ['  target_link_libraries(ispa_targets INTERFACE',
                  *('    ' + cmake_quote(path) for path in libraries), '  )']
    lines += ['endif()']
    for name, paths in sorted(targets.items()):
        var = re.sub('[^A-Za-z0-9_]', '_', name).upper()
        lines.append(f'set(ISPA_TARGET_{var}_DIRS ' + ' '.join(cmake_quote(path) for path in paths) + ')')
    (output / 'targets.cmake').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    return output, manifest


def export_variables(root, output, manifest):
    variables = {
        'ISPA_TARGETS_MANIFEST': str(output / 'targets.json'),
        'ISPA_TARGETS_CMAKE': str(output / 'targets.cmake'),
        'ISPA_CPP_INCLUDE_DIRS': os.pathsep.join(manifest['cpp']['include_dirs']),
        'ISPA_CPP_SOURCES': os.pathsep.join(manifest['cpp']['sources']),
        'ISPA_CPP_LIBRARY_DIRS': os.pathsep.join(manifest['cpp']['library_dirs']),
        'ISPA_CPP_LIBRARIES': os.pathsep.join(manifest['cpp']['libraries']),
        'ISPA_CPP_CORE_LIBRARIES': os.pathsep.join(manifest['cpp'].get('core_libraries', [])),
        'ISPA_PYTHON_INTERPRETER_DIRS': os.pathsep.join(manifest['python']['interpreter_dirs']),
    }
    for name, paths in sorted(manifest['targets'].items()):
        var = re.sub('[^A-Za-z0-9_]', '_', name).upper()
        variables['ISPA_TARGET_' + var + '_DIRS'] = os.pathsep.join(paths)
    return variables


def path_exports(manifest, shell):
    cpp = manifest['cpp']
    includes = cpp['include_dirs']
    libraries = cpp['library_dirs']
    runtime = cpp['runtime_dirs']
    python = manifest['python']['interpreter_dirs']
    if shell == 'powershell':
        return {'INCLUDE': includes, 'LIB': libraries, 'PATH': runtime,
                'CPLUS_INCLUDE_PATH': includes, 'LIBRARY_PATH': libraries,
                'PYTHONPATH': python}
    loader = 'DYLD_LIBRARY_PATH' if sys.platform == 'darwin' else 'LD_LIBRARY_PATH'
    return {'CPLUS_INCLUDE_PATH': includes, 'CPATH': includes,
            'LIBRARY_PATH': libraries, loader: runtime,
            'CMAKE_INCLUDE_PATH': includes, 'CMAKE_LIBRARY_PATH': libraries,
            'PYTHONPATH': python}


def shell_exports(variables, shell, manifest):
    if shell in ('bash', 'zsh'):
        lines = ['# Generated by ispa-pkg --export.']
        for name, value in variables.items():
            lines.append(f'export {name}={shlex.quote(value)}')
        for name, paths in path_exports(manifest, shell).items():
            for path in reversed(paths):
                lines += [
                    f'_ispa_path_entry={shlex.quote(path)}',
                    f'case ":${{{name}:-}}:" in',
                    '  *":${_ispa_path_entry}:"*) ;;',
                    f'  *) export {name}="${{_ispa_path_entry}}${{{name}:+:${{{name}}}}}" ;;',
                    'esac',
                ]
        lines.append('unset _ispa_path_entry')
        return '\n'.join(lines) + '\n'
    if shell == 'powershell':
        lines = ['# Generated by ispa-pkg --export.']
        for name, value in variables.items():
            lines.append(f"$env:{name} = '{value.replace(chr(39), chr(39)*2)}'")
        for name, paths in path_exports(manifest, shell).items():
            for path in reversed(paths):
                escaped = path.replace("'", "''")
                # One statement per line so the output also works piped into Invoke-Expression.
                lines.append(
                    f"if (-not (($env:{name} -split ';') -contains '{escaped}')) "
                    f"{{ $env:{name} = '{escaped}' + $(if ($env:{name}) {{ ';' + $env:{name} }}) }}")
        return '\n'.join(lines) + '\n'
    raise PackageError(f'Unsupported shell: {shell}')


def default_shell():
    if os.name == 'nt':
        return 'powershell'
    name = Path(os.environ.get('SHELL', '/bin/bash')).name
    return name if name in ('bash', 'zsh') else 'bash'


def write_shell_export(root, shell, variables, manifest):
    script = root / '.ispa-exports' / ('env.ps1' if shell == 'powershell' else 'env.sh')
    script.write_text(shell_exports(variables, shell, manifest), encoding='utf-8')
    return script


def windows_permanent_export(variables, manifest):
    """Persist exports in the user environment (HKCU\\Environment), visible to every new process."""
    import ctypes
    import winreg
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER, 'Environment') as key:
        def read(name):
            try:
                return winreg.QueryValueEx(key, name)
            except FileNotFoundError:
                return '', winreg.REG_SZ
        for name, value in variables.items():
            winreg.SetValueEx(key, name, 0, winreg.REG_SZ, value)
        for name, paths in path_exports(manifest, 'powershell').items():
            current, kind = read(name)
            existing = [p for p in current.split(';') if p]
            known = {p.lower() for p in existing}
            added = [p for p in paths if p.lower() not in known]
            if added:
                winreg.SetValueEx(key, name, 0, kind if kind in (winreg.REG_SZ, winreg.REG_EXPAND_SZ) else winreg.REG_SZ,
                                  ';'.join(added + existing))
    result = ctypes.c_ulong()
    ctypes.windll.user32.SendMessageTimeoutW(0xFFFF, 0x001A, 0, 'Environment', 0x2, 5000, ctypes.byref(result))


def permanent_export(root, shell, variables, manifest):
    if shell in ('bash', 'zsh'):
        profile = Path.home() / ('.bashrc' if shell == 'bash' else '.zshrc')
        script = write_shell_export(root, shell, variables, manifest)
        start = '# >>> ispa-pkg exports >>>'
        end = '# <<< ispa-pkg exports <<<' 
        block = '\n'.join((
            start,
            f'[ -f {shlex.quote(str(script))} ] && . {shlex.quote(str(script))}',
            'ispa-pkg() {',
            '  local _ispa_pkg_arg _ispa_pkg_file',
            '  for _ispa_pkg_arg in "$@"; do',
            '    if [ "$_ispa_pkg_arg" = "--export" ]; then',
            '      _ispa_pkg_file=$(command ispa-pkg "$@" --export-path) || return $?',
            '      [ -n "$_ispa_pkg_file" ] || return 1',
            '      . "$_ispa_pkg_file"',
            '      return $?',
            '    fi',
            '  done',
            '  command ispa-pkg "$@"',
            '}',
            end,
        ))
    elif shell == 'powershell':
        # Use the standard per-user cross-platform PowerShell profile location.
        profile = (Path.home() / 'Documents' / 'PowerShell' / 'Microsoft.PowerShell_profile.ps1')
        script = write_shell_export(root, shell, variables, manifest)
        start = '# >>> ispa-pkg exports >>>'
        end = '# <<< ispa-pkg exports <<<' 
        escaped = str(script).replace("'", "''")
        block = '\n'.join((
            start,
            f"if (Test-Path '{escaped}') {{ . '{escaped}' }}",
            'function global:ispa-pkg {',
            '  $ispaCommand = (Get-Command ispa-pkg -CommandType Application | Select-Object -First 1).Source',
            "  if ($args -contains '--export') {",
            "    $ispaArgs = @($args) + @('--export-path')",
            '    $ispaFile = & $ispaCommand @ispaArgs',
            '    if ($LASTEXITCODE -ne 0) { return }',
            '    . $ispaFile',
            '  } else {',
            '    & $ispaCommand @args',
            '  }',
            '}',
            end,
        ))
    else:
        raise PackageError(f'Unsupported shell: {shell}')
    profile.parent.mkdir(parents=True, exist_ok=True)
    old = profile.read_text(encoding='utf-8') if profile.exists() else ''
    if start in old or end in old:
        if old.count(start) != 1 or old.count(end) != 1 or old.index(start) > old.index(end):
            raise PackageError(f'Malformed ispa-pkg markers in {profile}')
        begin, finish = old.index(start), old.index(end) + len(end)
        updated = old[:begin] + block + old[finish:]
    else:
        updated = old.rstrip('\n') + ('\n\n' if old.strip() else '') + block + '\n'
    profile.write_text(updated, encoding='utf-8')
    return profile, script



def refresh_saved_exports(root):
    """Keep saved export scripts current after installs and updates."""
    output = root / '.ispa-exports'
    sh = output / 'env.sh'
    ps = output / 'env.ps1'
    if not sh.exists() and not ps.exists():
        return
    output, manifest = write_export_files(root)
    variables = export_variables(root, output, manifest)
    if sh.exists():
        sh.write_text(shell_exports(variables, 'bash', manifest), encoding='utf-8')
    if ps.exists():
        ps.write_text(shell_exports(variables, 'powershell', manifest), encoding='utf-8')


def main(argv=None):
    parser = argparse.ArgumentParser(prog="ispa-pkg", description=__doc__)
    parser.add_argument("--root", type=Path, default=default_root())
    parser.add_argument("--catalog", type=Path, help="Local YAML catalog path")
    exports = parser.add_mutually_exclusive_group()
    exports.add_argument("--export", action="store_true", help="Print shell exports and generate target manifests")
    exports.add_argument("--export-permanent", action="store_true", help="Persist exports in the user shell profile")
    exports.add_argument("--build", metavar="PACKAGE", help="Build an installed package's C++ library and Rust crates")
    parser.add_argument("--export-path", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--shell", choices=("bash", "zsh", "powershell"), help="Shell used for exports")
    parser.add_argument("--verbose", action="store_true",
                         help="Print each install/build step on its own line instead of a single status line")
    sub = parser.add_subparsers(dest="command")
    tool = sub.add_parser("tool", help="Install a compiled ISPA tool")
    tool.add_argument("name", choices=("antlr-to-ispa", "flex-to-ispa", "bison-to-ispa"))
    tool.add_argument("--version", help="Release tag (default: latest)")
    tool.add_argument("--bin-dir", type=Path, help="Executable destination (default: ROOT/../bin)")
    tool.add_argument("--force", action="store_true", help="Replace an installed executable")
    add = sub.add_parser("install", help="Install a catalog name, GitHub repository, or archive")
    add.add_argument("source")
    add.add_argument("--name", help="Installed name for a direct source")
    add.add_argument("--ref", help="GitHub branch, tag, or commit")
    add.add_argument("--allow-scripts", action="store_true")
    add.add_argument("--force", action="store_true")
    update = sub.add_parser("update", help="Update installed packages")
    update.add_argument("names", nargs="*")
    update.add_argument("--allow-scripts", action="store_true")
    doctor = sub.add_parser("doctor", help="Reinstall libraries built for a different generator version")
    doctor.add_argument("--allow-scripts", action="store_true")
    sub.add_parser("list", help="List installed packages")
    catalog_cmd = sub.add_parser("catalog", help="Manage the YAML catalog")
    catalog_cmd.add_argument("action", choices=["update", "list"])
    catalog_cmd.add_argument("--url", default=CATALOG_URL, help="GitHub raw YAML URL")
    args = parser.parse_args(argv)
    STATUS.verbose = args.verbose
    if args.export_path and not args.export:
        parser.error("--export-path requires --export")
    root = args.root.expanduser().resolve()
    if args.command is None and not (args.export or args.export_permanent or args.build):
        parser.error("Specify a command, --build, --export, or --export-permanent")
    if args.command is not None and (args.export or args.export_permanent or args.build):
        parser.error("Export flags cannot be combined with a subcommand")
    try:
        if args.command == "tool":
            from .tools import install_tool
            install_tool(args.name, root, args.version, args.bin_dir, args.force)
            return 0
        if args.build:
            if not NAME.fullmatch(args.build):
                raise PackageError(f"Invalid package name: {args.build}")
            package = root / args.build
            marker = package / ".ispa-package.json"
            if not marker.is_file():
                raise PackageError(f"Package not installed: {args.build}")
            archive = build_cpp_library(package, args.build, root)
            core_archive = build_cpp_core(package, args.build, root)
            rust_crates = build_rust_package(package, args.build, required=True)
            if archive or core_archive or rust_crates:
                refresh_saved_exports(root)
                STATUS.done()
                if archive:
                    print(f"{args.build}: built {archive}")
                if core_archive:
                    print(f"{args.build}: built {core_archive}")
                if rust_crates:
                    print(f"{args.build}: built {len(rust_crates)} Rust crate(s)")
            else:
                raise PackageError(f"Package has no C++ sources or Rust crates to build: {args.build}")
            return 0
        if args.export or args.export_permanent:
            output, manifest = write_export_files(root)
            variables = export_variables(root, output, manifest)
            shell = args.shell or default_shell()
            if args.export_permanent and os.name == 'nt':
                windows_permanent_export(variables, manifest)
                write_shell_export(root, 'powershell', variables, manifest)
                print("Saved exports to the user environment (HKCU\\Environment).")
                print(f"CMake targets: {output / 'targets.cmake'}")
                print("Open a new terminal for the changes to take effect.")
            elif args.export_permanent:
                profile, script = permanent_export(root, shell, variables, manifest)
                print(f"Saved export script: {script}")
                print(f"Updated shell profile: {profile}")
                print(f"CMake targets: {output / 'targets.cmake'}")
                print("Open a new shell to use ispa-pkg --export in the current session.")
            else:
                script = write_shell_export(root, shell, variables, manifest)
                if args.export_path:
                    print(script)
                else:
                    print(shell_exports(variables, shell, manifest), end='')
            return 0
        if args.command == "catalog":
            if args.action == "update":
                if not args.url.startswith("https://"):
                    raise PackageError("Catalog URL must use HTTPS")
                data = request(args.url)
                parsed = read_yaml(data, args.url)
                if not isinstance(parsed.get("packages"), dict):
                    raise PackageError("Catalog must contain a packages mapping")
                path = args.catalog or root / "catalog.yaml"
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
                print(f"Updated catalog: {path}")
            else:
                for name in catalog(root, args.catalog).get("packages", {}):
                    print(name)
        elif args.command == "doctor":
            version = generator_version(required=True)
            # Dependencies are repaired by install before their dependents are built.
            for marker in sorted(root.glob("*/.ispa-package.json")):
                data = json.loads(marker.read_text())
                name = marker.parent.name
                if data.get("generator_version") == version:
                    print(f"{name}: compatible with generator {version}")
                    continue
                print(f"{name}: reinstalling for generator {version} "
                      f"(was {data.get('generator_version', 'unversioned')})")
                install(name, installed_source(name, data, root, args.catalog), root,
                        args.allow_scripts, force=True, from_catalog=data.get("from_catalog", False),
                        catalog_path=args.catalog)
        elif args.command == "list":
            for marker in sorted(root.glob("*/.ispa-package.json")):
                data = json.loads(marker.read_text())
                print(f"{data['name']}\t{data['revision'][:12]}\t{marker.parent}")
        elif args.command == "install":
            packages = catalog(root, args.catalog).get("packages", {})
            spec = packages.get(args.source) if isinstance(packages, dict) else None
            spec = dict(spec) if isinstance(spec, dict) else parse_source(args.source)
            if args.ref:
                if "github" not in spec:
                    raise PackageError("--ref applies only to GitHub repositories")
                spec["ref"] = args.ref
            name = args.name or (args.source if args.source in packages else Path(args.source.rstrip("/")).name.removesuffix(".git").split(".")[0])
            install(name, spec, root, args.allow_scripts, args.force, args.source in packages and not args.ref, args.catalog)
        else:
            markers = list(root.glob("*/.ispa-package.json"))
            installed = {p.parent.name: json.loads(p.read_text()) for p in markers}
            names = args.names or sorted(installed)
            for name in names:
                if name not in installed:
                    raise PackageError(f"Package not installed: {name}")
                spec = installed_source(name, installed[name], root, args.catalog)
                install(name, spec, root, args.allow_scripts, from_catalog=installed[name].get("from_catalog", False),
                        catalog_path=args.catalog)
        return 0
    except (PackageError, OSError, ValueError, subprocess.CalledProcessError, zipfile.BadZipFile, tarfile.TarError) as error:
        STATUS.done()
        print(f"ispa-pkg: {error}", file=sys.stderr)
        return 1


def run():
    raise SystemExit(main())
