"""Run host-side C regression tests without a keyboard or QMK dependencies.

Usage: python path/to/aefreedman/tests/run.py [--cc path/to/gcc]
Requires GCC or Clang. Tests mock QMK calls; a firmware build and hardware
smoke test are still needed for full validation. Build artifacts are temporary.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", help="GCC/Clang executable")
    args = parser.parse_args()
    compiler = args.cc or shutil.which("gcc") or shutil.which("clang")
    if not compiler and Path("C:/msys64/mingw64/bin/gcc.exe").is_file():
        compiler = "C:/msys64/mingw64/bin/gcc.exe"
    if not compiler:
        parser.error("GCC or Clang is required; specify --cc")

    tests = Path(__file__).resolve().parent
    repo = next(p for p in tests.parents if (p / "quantum/keycodes.h").is_file())
    keymap = (tests.parent / "keymap.c").read_text(encoding="utf-8")
    # Compile the production callback bodies, not a separately maintained copy.
    declarations = keymap.split("typedef enum {\n    TD_NONE", 1)[1].split("#define TH_NUM_BSPC", 1)[0]
    callbacks = keymap.split("static void queue_oneshot_mods(uint8_t mods) {", 1)[1].split("\ntap_dance_action_t tap_dance_actions[]", 1)[0]
    source = "typedef enum {\n    TD_NONE" + declarations
    source += "static void queue_oneshot_mods(uint8_t mods) {" + callbacks

    env = os.environ.copy()
    # Windows GCC installations may need their adjacent runtime DLLs on PATH.
    env["PATH"] = str(Path(compiler).resolve().parent) + os.pathsep + env.get("PATH", "")
    with tempfile.TemporaryDirectory(prefix="reviung-tests-") as directory:
        build = Path(directory)
        (build / "tap_dance_under_test.inc").write_text(source, encoding="utf-8")
        cases = [
            ("tap_dances", "test_tap_dances.c", []),
        ]
        for name, filename, defines in cases:
            binary = build / (name + (".exe" if os.name == "nt" else ""))
            subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", *defines,
                 "-I", str(tests), "-I", str(build), "-I", str(repo / "quantum"),
                 str(tests / filename), "-o", str(binary)],
                check=True, env=env,
            )
            subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
