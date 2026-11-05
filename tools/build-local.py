#!/usr/bin/env python3
"""Build and test a project-local stage without installing system files.

Fallback for environments without Meson. Meson remains the canonical build.
Plugins and i18n are disabled. Requires Python 3 and a C11 compiler;
nondefault toolkits additionally require their development packages/pkg-config.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def run(argv, **kwargs):
    print("+", shlex.join(map(str, argv)), flush=True)
    return subprocess.run(list(map(str, argv)), check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tui", choices=["python", "curses"], default="python")
    parser.add_argument("--gui", choices=["html", "gtk", "qt"], default="html")
    parser.add_argument("--builddir", type=Path, default=ROOT / "build-local")
    args = parser.parse_args()
    build = args.builddir.resolve()
    if build == ROOT or ROOT in build.parents and build.parts[len(ROOT.parts)] in {"src", "data", "tests", "docs", "tools", "debian"}:
        parser.error("builddir must be separate from source directories")
    build.mkdir(parents=True, exist_ok=True)
    stage = build / "stage"
    for part in ("bin", "lib/pkgconfig", "include", "share"):
        (stage / part).mkdir(parents=True, exist_ok=True)
    cc = shlex.split(os.environ.get("CC", "cc"))
    cxx = shlex.split(os.environ.get("CXX", "c++"))

    def pkg_flags(modules, flag):
        pkg = shutil.which("pkg-config") or shutil.which("pkgconf")
        if not pkg:
            raise SystemExit("Missing pkg-config/pkgconf for selected toolkit")
        return shlex.split(subprocess.check_output([pkg, flag, *modules], text=True))

    tui_cflags, tui_libs, gui_cflags, gui_libs = [], [], [], []
    if args.tui == "curses":
        try:
            tui_cflags = pkg_flags(["ncursesw"], "--cflags")
            tui_libs = pkg_flags(["ncursesw"], "--libs")
        except subprocess.CalledProcessError:
            tui_cflags = pkg_flags(["ncurses"], "--cflags")
            tui_libs = pkg_flags(["ncurses"], "--libs")
    if args.gui != "html":
        modules = ["gtk4 >= 4.6"] if args.gui == "gtk" else ["Qt6Widgets"]
        gui_cflags = pkg_flags(modules, "--cflags")
        gui_libs = pkg_flags(modules, "--libs")

    values = {
        "CUDDLE_VERSION": "1.4.0", "CUDDLE_PREFIX": str(stage),
        "CUDDLE_BINDIR": str(stage / "bin"),
        "CUDDLE_DATADIR": str(stage / "share/cuddle"),
        "CUDDLE_PYTHON": sys.executable,
        "CUDDLE_LOCALEDIR": str(stage / "share/locale"),
        "CUDDLE_GETTEXT_DOMAIN": "cuddle",
    }
    config = "#pragma once\n" + "".join(f"#define {k} {json.dumps(v)}\n" for k, v in values.items())
    config += "".join(f"#define CUDDLE_HAS_{name} 1\n" for name in ["CLI", "TUI", "GUI", "USER_CONFIG"])
    (build / "config.h").write_text(config)
    common = ["-D_POSIX_C_SOURCE=200809L", "-Wall", "-Wextra", "-g", "-O2", "-I" + str(ROOT / "src/lib"), "-I" + str(build)]
    cflags = ["-std=c11", *common]
    lib = stage / "lib/libcuddle.so.1.4.0"
    run([*cc, *cflags, "-fPIC", "-shared", "-Wl,-soname,libcuddle.so.0",
         *sorted((ROOT / "src/lib").glob("*.c")), "-o", lib])
    for name in ["libcuddle.so.0", "libcuddle.so"]:
        link = lib.parent / name
        if link.is_symlink():
            link.unlink()
        link.symlink_to(lib.name)
    for name in ["cuddle.h", "cuddle-plugin.h"]:
        shutil.copy2(ROOT / "src/lib" / name, stage / "include" / name)
    shutil.copytree(ROOT / "data/cuddle", stage / "share/cuddle", dirs_exist_ok=True)
    (stage / "lib/pkgconfig/cuddle.pc").write_text(
        f"prefix={stage}\nlibdir=${{prefix}}/lib\nincludedir=${{prefix}}/include\n"
        "Name: cuddle\nDescription: cuddle core library\nVersion: 1.4.0\n"
        "Libs: -L${libdir} -lcuddle\nCflags: -I${includedir}\n")
    linkflags = ["-L" + str(lib.parent), "-lcuddle", "-Wl,-rpath,$ORIGIN/../lib"]
    run([*cc, *cflags, ROOT / "src/cli/main.c", ROOT / "src/cli/flood.c", ROOT / "src/cli/room.c", *linkflags, "-o", stage / "bin/cuddle"])
    tui_src = "cuddle-tui.c" if args.tui == "python" else "cuddle-tui-curses.c"
    run([*cc, *cflags, *tui_cflags, ROOT / "src/tui" / tui_src, *linkflags, *tui_libs, "-o", stage / "bin/cuddle-tui"])
    gui_src = {"html": "cuddle-gui.c", "gtk": "cuddle-gui-gtk.c", "qt": "cuddle-gui-qt.cpp"}[args.gui]
    compiler, flags = (cxx, ["-std=c++17", *common]) if args.gui == "qt" else (cc, cflags)
    run([*compiler, *flags, *gui_cflags, ROOT / "src/gui" / gui_src, *linkflags, *gui_libs, "-o", stage / "bin/cuddle-gui"])
    with tempfile.TemporaryDirectory(prefix="cuddle-test-", dir=build) as temp:
        env = dict(os.environ, XDG_CONFIG_HOME=temp, CUDDLE_SHARE=str(stage / "share/cuddle"), CUDDLE_BINDIR=str(stage / "bin"))
        for src in sorted((ROOT / "tests").glob("test_*.c")):
            exe = build / src.stem
            run([*cc, *cflags, src, "-L" + str(lib.parent), "-lcuddle", "-Wl,-rpath," + str(lib.parent), "-o", exe])
            run([exe], env=env)
        for name in ["cuddle", "cuddle-tui", "cuddle-gui"]:
            for flag in ["--help", "--version"]:
                run([stage / "bin" / name, flag], env=env)
        run(["sh", ROOT / "tests/check_help.sh", stage / "bin/cuddle"], env=env)
        for argv in [["cuddle", "--flood"], ["cuddle-tui"]]:
            result = subprocess.run([str(stage / "bin" / argv[0]), *argv[1:]], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
            if result.returncode != 2:
                raise RuntimeError(f"TTY rejection failed: {argv}: {result.returncode}")
    (build / "build-manifest.json").write_text(json.dumps({"tui": args.tui, "gui": args.gui, "prefix": str(stage), "plugins": False, "i18n": False}, indent=2) + "\n")
    print("Build and noninteractive tests passed. Staged in", stage)


if __name__ == "__main__":
    main()
