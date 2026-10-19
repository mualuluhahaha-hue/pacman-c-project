# Group information

MFE113TC - Programming Foundation for Intelligent Manufacturing Engineering
Group Project - Pac-Man (Text-Based, Standard C Only)

| item | value |
|------|-------|
| Course | MFE113TC - Programming Foundation for Intelligent Manufacturing Engineering |
| Project | Group Project - Pac-Man (Text-Based, Standard C Only) |
| Group | **Group 6** |
| Contribution to the overall marks | 35 % |
| Submission archive | `Group6 PacmanC.zip` |

## Members and individual parts

The report is organised as one shared outline (chapter 1) plus four independent
member chapters (chapters 2-5), so that the contribution of every member is
visible on its own. Chapter 6 is the shared integration chapter.

| member | student ID | part (brief) | code owned | report chapter |
|--------|-----------|--------------|------------|----------------|
| Boyang.Bian | 2577254 | Part 1 - Map & Rendering | `map.c`, `map.h`, `render.c`, `render.h`, both embedded mazes, `tests/cases/map_*.txt` | 2 |
| Zehao.Xin | 2580224 | Part 2 - Player Control & Rules | `entity.c`, `entity.h`, `input.c`, `input.h`, movement + collision rules | 3 |
| Siqi.Fan | 2581778 | Part 3 - Ghost Logic | `ghost.c`, `ghost.h` (random walk, scared ghosts, chase/scatter) | 4 |
| Grigorii Vorobev | 2582036 | Part 4 - Game State & Testing | `game.c`, `game.h`, `main.c`, `rng.c`, `rng.h`, whole test suite, integration, report build | 5 |

Shared work: chapter 1 (outline), chapter 6 (integration, contribution table,
limitations, reflection), the frozen `.h` interfaces and the final review of the
test evidence.

## If something has to be changed later

1. **Code change** - edit the source, then re-run the tests:
   `tests\run_tests.bat` (or `make test`). If a change alters a recorded log on
   purpose, re-record it with `python tests\run_tests.py --update`.
2. **Names / group id** - they are stored once, in `report/make_report.py`
   (constants `GROUP_ID`, `MEMBERS`, `IDS`) and in `make_zip.py` (`GROUP_ID`).
   Change them there and re-generate the report:
   `python report\make_report.py`, then convert `report/report.docx` to
   `report/report.pdf` (Word: *File > Save as > PDF*, or LibreOffice).
3. **Rebuild the archive** - `python make_zip.py` writes
   `..\Group6 PacmanC.zip` (one level above the project folder). Compiled
   binaries are deliberately not packed; the marker builds with
   `gcc *.c -o pacman`.

## What is verified

* the game builds from the project root with `gcc -std=c99 -Wall -Wextra
  -pedantic *.c -o pacman` and produces no warning;
* `unit_tests.exe` runs 294 checks and all of them pass;
* `tests/run_tests.py` plays 19 scripted games and every console log matches the
  recorded reference log;
* the same seed produces a byte identical log, a different seed does not;
* the contents of `Group6 PacmanC.zip` were extracted into an empty folder,
  rebuilt from scratch and tested again - all tests pass there as well.
