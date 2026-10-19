# Pac-Man (text-based, standard C only)

**MFE113TC – Programming Foundation for Intelligent Manufacturing Engineering**
Group project, 35 % of the overall marks. **Group 6**

A complete Pac-Man game that runs in the terminal. It is written in **standard C
only** (no graphics, no external libraries) and the build is the one the brief
suggests:

```
gcc *.c -o pacman
```

---

## 1. Quick start

### Windows (cmd / PowerShell)

```bat
build.bat                      :: or:  gcc *.c -o pacman.exe
pacman.exe                     :: play with the default settings
pacman.exe --seed 42 --ghosts 2
tests\run_tests.bat            :: build + unit tests + integration tests
```

### Linux / macOS

```sh
make                           # or:  gcc *.c -o pacman
./pacman --seed 42 --ghosts 2
sh tests/run_tests.sh          # build + unit tests + integration tests
```

### How to play

When the program starts it asks for a difficulty (type `1`–`5`, or `q` to quit).
Any other key is ignored and the question is asked again:

| difficulty | levels to clear | ghosts |
|-----------|-----------------|--------|
| 1 easy | 1 | 2 |
| 2 normal | 2 | 2 |
| 3 hard | 3 | 3 |
| 4 harder | 4 | 3 |
| 5 expert | 5 | 4 |

`--difficulty N` skips the menu, which is what the automated tests use.

Then type a key and press **Enter** (the game is tick based, so it waits for you):

| key | action |
|-----|--------|
| `w` | move up |
| `s` | move down |
| `a` | move left |
| `d` | move right |
| `q` | quit |

Any other key is ignored and never costs a tick. `Ctrl+Z` + Enter (Windows) or
`Ctrl+D` (Linux) ends the input stream and the game exits cleanly.

Board symbols: `#` wall, `.` pellet (10 points), `O` power pellet (50 points,
scares the ghosts), `P` Pac-Man, `G` ghost, `!` scared ghost (only after a power
pellet), `*` bonus fruit.

---

## 2. Command line options

| option | meaning |
|--------|---------|
| `--seed N` | random seed. The same seed always produces the same game (default `1`) |
| `--ghosts N` | number of ghosts, 1–4 (default `2`) |
| `--levels N` | number of levels, 1–5 (default `2`, an extension) |
| `--difficulty N` | 1–5, skip the start menu and use that row of the difficulty table |
| `--map FILE` | load a level from a plain text file (extension) |
| `--ai MODE` | `random` (default, brief rules), `pure` (uniform including reversal), `smart` (heuristic chase/scatter) |
| `--follow-ticks N` | anti-follow rule: turn a ghost round after N ticks of being followed (default 3) |
| `--follow-range N` | how many cells behind Pac-Man still counts as following (default 5) |
| `--no-antifollow` | switch the anti-follow rule off (useful to replay the old behaviour) |
| `--core` | switch **all** optional extensions off → exactly the core requirements |
| `--max-ticks N` | stop automatically after N ticks (used by the tests) |
| `--quiet` | do not print the board on every tick |
| `--check-map FILE` | validate a map file and exit |
| `--check-maps` | validate every embedded level and exit |
| `--help`, `--version` | usage / version |

Exit codes: `0` normal, `2` bad usage, `3` map error.

Examples:

```bat
pacman.exe --core --seed 7 --ghosts 1          :: core requirements only
pacman.exe --ai smart --seed 5 --ghosts 4      :: all extensions, smart ghosts
pacman.exe --levels 5 --seed 3                 :: play all five mazes
pacman.exe --check-maps                        :: validate every embedded level
pacman.exe --check-map tests\cases\map_win.txt :: validate a map file
```

---

## 3. Rules implemented

| rule | value |
|------|-------|
| lives | 3, one life is lost when Pac-Man and a normal ghost share a cell |
| pellet | +10, removed from the board |
| power pellet | +50, ghosts are scared for 20 ticks, Pac-Man may eat them |
| eating a scared ghost | 200 / 400 / 800 / 1600 points in one chain, respawn after 5 ticks |
| fruit | appears every 40 ticks, stays 15 ticks, +100 points |
| win | all pellets of the last level eaten |
| lose | lives reach 0 |
| death | Pac-Man and the ghosts return to their start cells, pellets stay eaten |
| levels | five hard-coded mazes, `--levels 1` … `--levels 5` (default 2) |
| anti-follow | a ghost that is followed for 3 ticks in the same corridor (up to 5 cells behind) turns round (see below) |

The exact tick order is documented in the comment at the top of `game.c` and in
the report (section 1.7): move → eat → collision → win check → timers → ghosts
move → collision.

### The anti-follow rule (why the ghosts turn round)

The brief's optional rule of thumb forbids a ghost to reverse while another
direction is open. That has a side effect: a player who stays one cell behind a
ghost can never be caught, because the ghost is not allowed to turn round.

The game therefore counts how long Pac-Man has been following a ghost. A ghost
turns round when Pac-Man has been

* in the **same corridor**, directly behind it, at most **5 cells** away
  (`--follow-range N`),
* with **no wall between** them (so the parallel corridor of a maze never
  counts), and
* for **3 ticks in a row** (`--follow-ticks N`).

The distance is what matters: Pac-Man and a ghost move one cell per tick, so a
player can keep *any* gap and stay safe forever. Counting the gap up to five
cells covers every distance a player naturally keeps, and because the check does
not look at how the gap changes, "step forward, wait, step forward" does not
help either.

Chasing a scared ghost is never affected, and `--no-antifollow` restores the
plain behaviour. Section 4 of the report measures both settings.

---

## 4. Files

| file | owner (brief section) | purpose |
|------|----------------------|---------|
| `main.c` | Part 4 – Game State & Testing | command line, tick loop, exit summary |
| `game.c` / `game.h` | Part 4 (+ Part 2 for collisions) | state, tick order, rules, summary |
| `map.c` / `map.h` | Part 1 – Map & Rendering | maps, board queries, integrity checks |
| `render.c` / `render.h` | Part 1 – Map & Rendering | ASCII board + status line |
| `entity.c` / `entity.h` | Part 2 – Player Control & Rules | positions, one-cell movement |
| `input.c` / `input.h` | Part 2 – Player Control & Rules | key handling, invalid keys, EOF |
| `ghost.c` / `ghost.h` | Part 3 – Ghost Logic | random walk, scared, chase/scatter |
| `rng.c` / `rng.h` | Part 4 (shared with Part 3) | xorshift32, reproducible randomness |
| `tests/unit_tests.c` | all four parts | 197 unit checks |
| `tests/run_tests.py` | Part 4 | 14 integration cases + determinism proof |
| `report/report.pdf` | all four parts | the group report |
| `CONTRIBUTIONS.md` | – | who wrote what |

---

## 5. Testing

```bat
tests\run_tests.bat            :: Windows, everything
sh tests/run_tests.sh          :: Linux / macOS, everything
make test                      :: same via make
python tests\run_tests.py      :: integration tests only
python tests\run_tests.py --update   :: re-record the reference logs
```

* **Unit tests** (`tests/unit_tests.c`): 197 checks over the RNG, map loading and
  validation, movement, ghost behaviour, the game rules, input handling and
  rendering. No Python required.
* **Integration tests** (`tests/run_tests.py`): 14 scripted games (win, two kinds
  of death, power pellet, fruit, wall bump, level change, invalid keys, EOF,
  smart AI …), each compared with a recorded reference log in `tests/expected/`.
  The run also proves that the same seed gives a byte-identical log and that a
  different seed does not.
* Every log ends with a machine readable line, for example:

  `RESULT=WIN SCORE=60 PELLETS_EATEN=6 PELLETS_TOTAL=6 TICKS=8 LIVES=3 LEVEL=1 SEED=42 GHOSTS=1`

Details and screenshots are in `report/report.pdf`, section 5.

---

## 6. Adding your own map

A map is plain text: one character per cell, `#` wall, `.` pellet, `O` power
pellet, `P` Pac-Man start, `G` ghost start, space = corridor. Blank lines are
ignored, `;` starts a comment. Maximum size is 25 rows × 40 columns and the map
must contain exactly one `P`, 1–4 `G` and at least one pellet.

```
#########
#P......#
#########
#   G   #
#########
```

Check it before playing:

```bat
pacman.exe --check-map mymap.txt
```

`--check-map` also proves that every pellet can actually be reached from
Pac-Man, so an impossible level cannot slip into the submission.
