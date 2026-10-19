#!/usr/bin/env python3
"""
make_zip.py - build the submission archive <GroupID> PacmanC.zip.

MFE113TC Group Project - text-based Pac-Man (standard C only)
Part 4 (Game State & Testing) - submission tooling.

    python make_zip.py                 # writes ../<GroupID> PacmanC.zip
    python make_zip.py --out C:\\tmp\\submission.zip

The archive contains exactly what the brief asks for - report (PDF), source
code and a short build/run guide - plus the test suite and the recorded
evidence.  Compiled binaries (*.exe, *.o) are left out on purpose: the marker
builds the project with "gcc *.c -o pacman".
"""

import argparse
import os
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
# Group id of the submission (part of the archive name, as the brief asks:
# "<GroupID> PacmanC.zip").
GROUP_ID = "Group6"

INCLUDE_ROOT = [
    "README.md", "GROUP_INFO.md", "CONTRIBUTIONS.md", "Makefile", "build.bat",
    "make_zip.py", "rng.h", "rng.c", "entity.h", "entity.c", "map.h", "map.c",
    "ghost.h", "ghost.c", "render.h", "render.c", "input.h", "input.c",
    "game.h", "game.c", "main.c",
]
INCLUDE_DIRS = [
    ("tests", ("unit_tests.c", "run_tests.py", "run_tests.bat", "run_tests.sh")),
    ("tests/cases", None),
    ("tests/expected", None),
    ("tests/logs", None),
    ("report", ("report.pdf", "report.docx", "make_report.py", "make_figures.py")),
    ("report/figures", None),
]
SKIP_SUFFIX = (".exe", ".o", ".obj", ".zip", ".tmp")


def collect():
    files = []
    for name in INCLUDE_ROOT:
        path = os.path.join(HERE, name)
        if os.path.isfile(path):
            files.append((path, name))
        else:
            print(f"  warning: {name} is missing")
    for folder, only in INCLUDE_DIRS:
        path = os.path.join(HERE, folder)
        if not os.path.isdir(path):
            print(f"  warning: {folder}/ is missing")
            continue
        for name in sorted(os.listdir(path)):
            full = os.path.join(path, name)
            if not os.path.isfile(full) or name.endswith(SKIP_SUFFIX):
                continue
            if only is not None and name not in only:
                continue
            files.append((full, f"{folder}/{name}"))
    return files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=None,
                        help="output archive (default: ../<GroupID> PacmanC.zip)")
    args = parser.parse_args()

    out = args.out or os.path.join(os.path.dirname(HERE), f"{GROUP_ID} PacmanC.zip")
    files = collect()

    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as zf:
        for full, arc in files:
            zf.write(full, arc)

    size = os.path.getsize(out)
    print(f"{out}")
    print(f"{len(files)} files, {size / 1024:.0f} KiB")
    for _, arc in files:
        print("   ", arc)


if __name__ == "__main__":
    main()
