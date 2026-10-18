# Who did what (contribution map)

MFE113TC group project (Group 6) - text-based Pac-Man (standard C only).

The brief recommends four roles. The code, the tests and the report are split
along exactly those four roles, and the report keeps them apart: chapter 1 is the
shared outline, chapters 2-5 are the four individual parts (one per member), and
chapter 6 is the shared integration chapter.

| part (brief) | member | student ID | source files owned | unit test section | report chapter |
|--------------|--------|-----------|--------------------|-------------------|----------------|
| Part 1 - Map & Rendering | Boyang.Bian | 2577254 | `map.c`, `map.h`, `render.c`, `render.h`, both embedded mazes, `tests/cases/map_*.txt` | `Part 1 / map`, `Part 1 / render` | 2 |
| Part 2 - Player Control & Rules | Zehao.Xin | 2580224 | `entity.c`, `entity.h`, `input.c`, `input.h`, collision rules in `game.c` | `Part 2 / entity`, `Part 2 / input` | 3 |
| Part 3 - Ghost Logic | Siqi.Fan | 2581778 | `ghost.c`, `ghost.h` | `Part 3 / ghost` | 4 |
| Part 4 - Game State & Testing | Grigorii Vorobev | 2582036 | `game.c`, `game.h`, `main.c`, `rng.c`, `rng.h`, `tests/`, `Makefile`, `build.bat`, `report/` | `Part 4 / rng`, `Part 4 / game` | 5 |

Shared work (all four members): chapter 1 (outline), chapter 6 (integration,
contribution table, limitations, reflection), the interface freeze of the `.h`
files and the final review of the test evidence.

## How to see each member's result on its own

1. **Report** - open `report/report.pdf`; each part starts on a new page with the
   member's name in the heading and ends with its own *individual test evidence*
   table and *individual result summary*.
2. **Code** - every source file starts with a header comment naming the part that
   owns it, so `grep "Part 3" *.c` lists the ghost work.
3. **Tests** - the unit suite prints section headers in part order
   (`Part 1 / map: ...`, `Part 3 / ghost: ...`), so a failing check names its
   owner directly.
4. **Integration** - `tests/run_tests.py` prints one line per case and writes one
   log per case into `tests/logs/`.
