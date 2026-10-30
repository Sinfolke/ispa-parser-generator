"""Generate, compile separately, and run external parser imports.

Run: python3 tests/extern_imports.py --ispa build/ispa --cxx 'clang++ -stdlib=libc++'
"""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--ispa", required=True, type=Path)
    cli.add_argument("--cxx", default="clang++")
    args = cli.parse_args()
    binary = args.ispa.resolve()
    repo = Path(__file__).resolve().parents[1]
    compiler = shlex.split(args.cxx)

    with tempfile.TemporaryDirectory(prefix="ispa-extern-") as temporary:
        root = Path(temporary)
        grammar = root / "grammar"
        grammar.mkdir()
        (grammar / "foreign.isc").write_text(
            "module expression -> value;\nWORD: @ [a-z]+ {@};\nvalue: @ WORD {@};\n"
        )
        (grammar / "host.isc").write_text(
            "module host -> main;\nimport extern foreign {value} from expression;\n"
            "main: '[' @ value ']' {@};\n"
        )

        def generate(*options):
            result = subprocess.run(
                [str(binary), "-l", "cpp", "-a", "LL", "--debug", "false",
                 "--no-nfa-debug", *map(str, options)], cwd=root,
                text=True, capture_output=True,
            )
            if result.returncode:
                raise AssertionError(result.stdout + result.stderr)

        def compile_and_run(output, dependency, extra=()):
            header = (output / "Host.h").read_text()
            assert f"#include <{dependency}.h>" in header
            assert str(root) not in header, "generated include embeds a machine path"
            assert f"::{dependency}::Types::modules::expression::value" in header
            assert ".exports.h" not in header
            driver = output / "run.cpp"
            driver.write_text("""
#include <Host.h>
#include <DEPENDENCY.h>
#include <cassert>
#include <type_traits>
int main() {
    Host::Parser parser;
    const auto& tree = parser.parse("[hello]");
    const auto& imported = tree.value.data().value;
    static_assert(std::is_same_v<typename std::remove_cvref_t<decltype(imported)>::data_type,
        DEPENDENCY::Types::modules::expression::value>);
    assert(parser.getErrors().empty());
    assert(PAYLOAD == "hello");
    assert(imported.startpos() == 1 && imported.length() == 5);
    bool rejected = false;
    try { Host::Lexer lexer; lexer.makeTokens("[hello]"); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
}
""".replace("DEPENDENCY", dependency).replace("PAYLOAD",
                "imported" + ".data().value" * (2 + len(extra))))
            includes = ["-I" + str(repo / "parser/stdlibs/target/cpp-core"), "-I" + str(output)]
            objects = []
            for name in (*extra, dependency, "Host"):
                obj = output / (name + ".o")
                subprocess.run(compiler + ["-std=c++23", *includes, "-c",
                               str(output / (name + ".cpp")), "-o", str(obj)], check=True)
                objects.append(str(obj))
            archive = output / "dependency.a"
            subprocess.run(["ar", "rcs", str(archive), *objects[:-1]], check=True)
            executable = output / "run"
            subprocess.run(compiler + ["-std=c++23", *includes, str(driver), objects[-1],
                           str(archive), str(repo / "parser/stdlibs/target/cpp-core/src/core.cpp"),
                           "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

        linked = root / "linked"
        generate("-o", linked / "Foreign", grammar / "foreign.isc")
        generate("-o", linked / "Host", "--dep", "foreign", linked / "Foreign",
                 "--dep", "unused", linked / "Foreign", grammar / "host.isc")
        compile_and_run(linked, "Foreign")

        # Installed manifests supply --dep without an explicit CLI binding.
        installed_root = root / "installation" / "libraries"
        installed = installed_root / "foreign" / "ispatarget" / "cpp"
        installed.mkdir(parents=True)
        for suffix in (".h", ".ispa-export"):
            shutil.copy2(linked / ("Foreign" + suffix), installed / ("Foreign" + suffix))
        env = os.environ.copy()
        env["ISPA_PKG_ROOT"] = str(installed_root)
        automatic_binding = subprocess.run(
            [str(binary), "-l", "cpp", "-a", "LL", "--debug", "false",
             "--no-nfa-debug", "-o", str(root / "auto-binding/Host"), str(grammar / "host.isc")],
            cwd=root, env=env, text=True, capture_output=True,
        )
        if automatic_binding.returncode:
            raise AssertionError(automatic_binding.stdout + automatic_binding.stderr)
        assert "#include <Foreign.h>" in (root / "auto-binding/Host.h").read_text()

        automatic = root / "automatic"
        options = ("--no-dep", "--dir", grammar, "-o", automatic / "Host", grammar / "host.isc")
        generate(*options)
        compile_and_run(automatic, "foreign")
        before = {p.name: p.read_bytes() for p in automatic.glob("*.cpp")}
        generate(*options)
        assert before == {p.name: p.read_bytes() for p in automatic.glob("*.cpp")}

        # A package importing another external package must be generated first.
        (grammar / "atom.isc").write_text(
            "module atom;\nWORD: @ [a-z]+ {@};\nvalue: @ WORD {@};\n"
        )
        (grammar / "foreign.isc").write_text(
            "module expression -> value;\nimport extern atom atom as atom;\n"
            "value: @ atom.value {@};\n"
        )
        nested = root / "nested"
        generate("--no-dep", "--dir", grammar, "-o", nested / "Host", grammar / "host.isc")
        assert (nested / "atom.cpp").is_file()
        assert "#include <atom.h>" in (nested / "foreign.h").read_text()
        compile_and_run(nested, "foreign", ("atom",))

        (grammar / "atom.isc").write_text(
            "module atom -> value;\nimport extern foreign expression as peer;\n"
            "value: @ peer.value {@};\n"
        )
        cycle = subprocess.run(
            [str(binary), "-l", "cpp", "--no-dep", "--dir", str(grammar),
             "-o", str(root / "cycle/Host"), str(grammar / "host.isc")],
            cwd=root, text=True, capture_output=True,
        )
        assert cycle.returncode != 0
        assert "Cyclic external parser dependency" in cycle.stdout + cycle.stderr

    print("External imports: separate library, original types, --dep, --no-dep, and nested generation passed")


if __name__ == "__main__":
    main()
