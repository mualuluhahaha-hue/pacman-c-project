#!/usr/bin/env python3
"""
run_tests.py - integration test runner for the MFE113TC Pac-Man project.

MFE113TC Group Project - text-based Pac-Man (standard C only)
Part 4 (Game State & Testing).

It runs the compiled game on scripted input, stores the console log in
tests/logs/, compares it with the recorded reference log in tests/expected/
and checks the machine readable summary line and a few arithmetic rules
(for example: in core mode the score must be exactly 10 x pellets eaten).

Usage:
    python tests/run_tests.py            # run every case
    python tests/run_tests.py --update   # re-record the reference logs
    python tests/run_tests.py win die    # run only the named cases

Exit code 0 = every case passed.
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
LOGS = os.path.join(HERE, "logs")
EXPECTED = os.path.join(HERE, "expected")

# name, argument list, input file, expected result keyword, extra checks
CASES = [
    dict(name="win", args=["--map", "tests/cases/map_win.txt", "--seed", "42",
                           "--ghosts", "1", "--quiet"],
         stdin="tests/cases/in_win.txt", result="WIN",
         checks=["win_all_pellets"]),
    dict(name="die_ghost_touches_pac", args=["--map", "tests/cases/map_die.txt",
                                             "--seed", "42", "--ghosts", "1", "--quiet"],
         stdin="tests/cases/in_die.txt", result="LOSE",
         checks=["lives_zero"]),
    dict(name="die_pac_touches_ghost", args=["--map", "tests/cases/map_die.txt",
                                             "--seed", "42", "--ghosts", "1", "--quiet"],
         stdin="tests/cases/in_die2.txt", result="LOSE",
         checks=["lives_zero"]),
    dict(name="power_pellet", args=["--map", "tests/cases/map_power.txt",
                                    "--seed", "3", "--ghosts", "1"],
         stdin="tests/cases/in_power.txt", result="QUIT",
         checks=["scared_seen"]),
    dict(name="board_render_demo", args=["--seed", "1", "--ghosts", "2",
                                         "--max-ticks", "12"],
         stdin="tests/cases/in_demo.txt", result="QUIT",
         checks=["render_seen"]),
    dict(name="pellet_counting_core", args=["--core", "--seed", "5", "--ghosts", "1",
                                            "--quiet"],
         stdin="tests/cases/in_demo.txt", result=None,
         checks=["core_score"]),
    dict(name="wall_bump_no_move", args=["--map", "tests/cases/map_win.txt",
                                         "--seed", "1", "--ghosts", "1", "--quiet"],
         stdin="tests/cases/in_bump.txt", result="WIN",
         checks=["bump_proof"]),
    dict(name="default_two_ghosts", args=["--seed", "1", "--ghosts", "2",
                                          "--quiet", "--max-ticks", "60"],
         stdin="tests/cases/in_demo.txt", result=None, checks=["deterministic"]),
    dict(name="default_smart_ai", args=["--seed", "1", "--ghosts", "2", "--ai", "smart",
                                        "--quiet", "--max-ticks", "60"],
         stdin="tests/cases/in_demo.txt", result=None, checks=["deterministic"]),
    dict(name="tailgate", args=["--map", "tests/cases/map_tail.txt",
                                "--seed", "1", "--ghosts", "1", "--quiet"],
         stdin="tests/cases/in_tail.txt", result="LOSE",
         checks=["antifollow_fired"]),
    dict(name="tailgate_off", args=["--map", "tests/cases/map_tail.txt",
                                    "--seed", "1", "--ghosts", "1", "--quiet",
                                    "--no-antifollow"],
         stdin="tests/cases/in_tail.txt", result="QUIT",
         checks=["antifollow_off"]),
    dict(name="level_transition", args=["--map", "tests/cases/map_win.txt",
                                        "--levels", "2", "--seed", "42",
                                        "--ghosts", "1", "--quiet"],
         stdin="tests/cases/in_win.txt", result="QUIT",
         checks=["level_two_seen", "embedded_level_two"]),
    dict(name="invalid_keys_ignored", args=["--seed", "1", "--ghosts", "1",
                                            "--quiet", "--max-ticks", "3"],
         stdin="tests/cases/in_invalid.txt", result="QUIT",
         checks=["invalid_seen", "ticks_three"]),
    dict(name="eof_no_crash", args=["--seed", "1", "--ghosts", "1", "--quiet"],
         stdin="tests/cases/in_eof.txt", result="QUIT",
         checks=["eof_message"]),
    # the three cases below run WITHOUT --difficulty on purpose, so the start
    # menu really asks its question
    dict(name="difficulty_menu", args=["--seed", "1", "--quiet",
                                       "--max-ticks", "3"], menu=True,
         stdin="tests/cases/in_menu.txt", result=None,
         checks=["difficulty_three"]),
    dict(name="difficulty_invalid_keys", args=["--seed", "1",
                                               "--quiet", "--max-ticks", "2"],
         menu=True, stdin="tests/cases/in_menu_invalid.txt", result=None,
         checks=["difficulty_two", "menu_invalid_seen"]),
    dict(name="difficulty_eof", args=["--seed", "1"], menu=True, menu_eof=True,
         stdin="tests/cases/in_menu_eof.txt", result=None, checks=[]),
]

PASSED = []
FAILED = []


def find_binary():
    for candidate in ("pacman.exe", "pacman"):
        path = os.path.join(ROOT, candidate)
        if os.path.isfile(path):
            return path
    return None


def run_case(binary, case):
    """Run one case and return (stdout text, return code)."""
    with open(os.path.join(ROOT, case["stdin"]), "r", encoding="utf-8") as fh:
        data = fh.read()
    args = list(case["args"])
    if not case.get("menu"):
        # skip the start menu, otherwise it would eat the first input line
        args += ["--difficulty", "2"]
    proc = subprocess.run([binary] + args, cwd=ROOT, input=data,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True, encoding="utf-8", errors="replace")
    return proc.stdout, proc.returncode


def summary_fields(text):
    """Parse the machine readable RESULT=... line into a dict."""
    fields = {}
    for line in text.splitlines():
        if line.startswith("RESULT="):
            for part in line.split():
                if "=" in part:
                    key, value = part.split("=", 1)
                    fields[key] = value
    return fields


def check_case(case, text, code):
    """Return a list of failure strings (empty list = all checks passed)."""
    problems = []

    if case.get("menu_eof"):
        # the game stops at the menu, so there is no RESULT= line on purpose
        if code != 0:
            problems.append(f"exit code {code}, expected 0")
        if "End of input" not in text:
            problems.append("no graceful end-of-input message at the start menu")
        return problems

    fields = summary_fields(text)

    if not fields:
        problems.append("no RESULT= summary line in the output")
        return problems

    if code != 0:
        problems.append(f"exit code {code}, expected 0")

    if case["result"] is not None and fields.get("RESULT") != case["result"]:
        problems.append(f"RESULT={fields.get('RESULT')}, expected {case['result']}")

    for check in case["checks"]:
        if check == "core_score":
            score = int(fields.get("SCORE", -1))
            eaten = int(fields.get("PELLETS_EATEN", -1))
            if score != 10 * eaten:
                problems.append(f"core mode: score {score} != 10 x {eaten} pellets")
        elif check == "win_all_pellets":
            if fields.get("PELLETS_EATEN") != fields.get("PELLETS_TOTAL"):
                problems.append("the win did not eat every pellet")
        elif check == "lives_zero":
            if fields.get("LIVES") != "0":
                problems.append("game over but lives left")
        elif check == "scared_seen":
            if "SCARED" not in text:
                problems.append("no SCARED status after the power pellet")
        elif check == "bump_proof":
            # in_bump.txt = 2 bumps against the wall + 6 moves that eat the
            # 6 pellets: 8 ticks and 6 pellets prove the bumps did not move
            if fields.get("TICKS") != "8" or fields.get("PELLETS_EATEN") != "6":
                problems.append("wall bumps changed the game state "
                                f"(ticks={fields.get('TICKS')}, "
                                f"pellets={fields.get('PELLETS_EATEN')})")
        elif check == "level_two_seen":
            if "LEVEL 2" not in text:
                problems.append("level 2 was never loaded")
        elif check == "embedded_level_two":
            if fields.get("LEVEL") != "2":
                problems.append("summary does not report level 2")
        elif check == "invalid_seen":
            if "Unknown key" not in text:
                problems.append("invalid keys were not reported")
        elif check == "ticks_three":
            if fields.get("TICKS") != "3":
                problems.append(f"ticks={fields.get('TICKS')}, expected 3 "
                                "(invalid keys must not cost a tick)")
        elif check == "eof_message":
            if "EOF" not in text:
                problems.append("no graceful EOF message")
        elif check == "difficulty_three":
            if fields.get("DIFFICULTY") != "3":
                problems.append(f"DIFFICULTY={fields.get('DIFFICULTY')}, "
                                "the menu answer 3 was not applied")
            if fields.get("GHOSTS") != "3":
                problems.append(f"GHOSTS={fields.get('GHOSTS')}, difficulty 3 "
                                "should have set three ghosts")
        elif check == "difficulty_two":
            if fields.get("DIFFICULTY") != "2":
                problems.append(f"DIFFICULTY={fields.get('DIFFICULTY')}, expected 2")
            if fields.get("GHOSTS") != "2":
                problems.append(f"GHOSTS={fields.get('GHOSTS')}, expected 2")
        elif check == "menu_invalid_seen":
            if "Invalid input" not in text:
                problems.append("invalid menu keys were not reported")
        elif check == "antifollow_fired":
            if int(fields.get("ANTIFOLLOW", 0)) < 1:
                problems.append("the anti-follow rule never fired while the "
                                "player was tailgating the ghost")
        elif check == "antifollow_off":
            if fields.get("ANTIFOLLOW") != "0":
                problems.append("the anti-follow rule fired although it was "
                                "switched off with --no-antifollow")
        elif check == "render_seen":
            if "Pellets" not in text or "########" not in text:
                problems.append("the board was not rendered")
        elif check == "deterministic":
            pass    # handled separately by the determinism case
    return problems


def main(argv):
    update = "--update" in argv
    wanted = [a for a in argv if not a.startswith("-")]
    binary = find_binary()
    if binary is None:
        print("ERROR: pacman binary not found - build it first "
              "(gcc *.c -o pacman)")
        return 1

    os.makedirs(LOGS, exist_ok=True)
    os.makedirs(EXPECTED, exist_ok=True)

    print("integration tests")
    print("=================")

    for case in CASES:
        if wanted and case["name"] not in wanted:
            continue
        name = case["name"]
        text, code = run_case(binary, case)
        log_path = os.path.join(LOGS, name + ".txt")
        with open(log_path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)

        exp_path = os.path.join(EXPECTED, name + ".txt")
        if update:
            with open(exp_path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)
            print(f"  {name:<24} recorded")
            continue

        problems = check_case(case, text, code)
        if os.path.isfile(exp_path):
            with open(exp_path, "r", encoding="utf-8") as fh:
                reference = fh.read()
            if reference != text:
                problems.append("console log differs from the reference log "
                                f"(tests/expected/{name}.txt)")
        else:
            problems.append("no reference log (run with --update once)")

        if problems:
            FAILED.append(name)
            print(f"  {name:<24} FAIL")
            for p in problems:
                print(f"      - {p}")
        else:
            PASSED.append(name)
            print(f"  {name:<24} PASS")

    # ---- determinism: the same seed must give a byte identical log --------
    if not wanted or "default_two_ghosts" in wanted:
        case = next(c for c in CASES if c["name"] == "default_two_ghosts")
        first, _ = run_case(binary, case)
        second, _ = run_case(binary, case)
        if first == second:
            PASSED.append("same_seed_identical")
            print(f"  {'same_seed_identical':<24} PASS")
        else:
            FAILED.append("same_seed_identical")
            print(f"  {'same_seed_identical':<24} FAIL")
            print("      - two runs with the same seed differ")

        other = dict(case)
        other["args"] = ["--seed", "2" if case["args"][1] == "1" else "1",
                         "--ghosts", "2", "--quiet", "--max-ticks", "60"]
        third, _ = run_case(binary, other)
        if third != first:
            PASSED.append("different_seed_differs")
            print(f"  {'different_seed_differs':<24} PASS")
        else:
            FAILED.append("different_seed_differs")
            print(f"  {'different_seed_differs':<24} FAIL")
            print("      - two different seeds produced the same log")

    print("=================")
    print(f"integration tests run: {len(PASSED) + len(FAILED)}, "
          f"failed: {len(FAILED)}")
    if FAILED:
        print("INTEGRATION TESTS: FAILED -> " + ", ".join(FAILED))
        return 1
    print("INTEGRATION TESTS: ALL PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
