#!/bin/sh
# ---------------------------------------------------------------------
# run_tests.sh - build everything and run the whole test suite (Linux/macOS).
# MFE113TC group project - text-based Pac-Man (standard C only)
# Part 4 (Game State & Testing).
# ---------------------------------------------------------------------
set -e
cd "$(dirname "$0")/.."

echo "====================================================="
echo " step 1: build the game"
echo "====================================================="
gcc -std=c99 -Wall -Wextra -pedantic -O2 *.c -o pacman

echo
echo "====================================================="
echo " step 2: build and run the unit tests"
echo "====================================================="
gcc -std=c99 -Wall -Wextra -pedantic -O2 -I. tests/unit_tests.c rng.c entity.c \
    map.c game.c ghost.c render.c input.c screen.c -o unit_tests
./unit_tests

echo
echo "====================================================="
echo " step 3: integration tests"
echo "====================================================="
if command -v python3 >/dev/null 2>&1; then
    python3 tests/run_tests.py
elif command -v python >/dev/null 2>&1; then
    python tests/run_tests.py
else
    echo "python was not found - running the diff based fallback checks"
    ./pacman --map tests/cases/map_win.txt --seed 42 --ghosts 1 --quiet \
        < tests/cases/in_win.txt > tests/logs/win.txt
    diff -u tests/expected/win.txt tests/logs/win.txt
    echo "fallback check passed"
fi

echo
echo "ALL TESTS PASSED"
