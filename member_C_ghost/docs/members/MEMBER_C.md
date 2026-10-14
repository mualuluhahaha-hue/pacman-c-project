# Member C - Ghost Logic

**Name / student ID:** ______________________   **Contribution:** ____ %

## My files
- src/ghost.c
- tests/test_ghost.c

## Functions I provide (used by teammates)
- ghost_step(g, e) - one random valid step of one ghost
- ghosts_move_all(g) - every ghost moves once, waiting ghosts count down
- ghosts_reset(g) - all ghosts back to their start cells
- ghost_at(g, r, c) - index of an active ghost on a cell, or -1

## Functions I use from teammates
- map_is_wall() from Member A
- struct Game / struct Entity from pacman.h

## Build and run my tests
```
gcc tests/test_ghost.c src/map.c src/player.c src/ghost.c src/engine.c -o test_ghost
./test_ghost
```
(The tests link all modules except main.c, so the other members' files must be in the repository too.)

## Lecture topics I used
rand() and srand() (Lecture 05), arrays (valid[4], DIR_R/DIR_C), const (Lecture 02), for loops and if (Lecture 04), pointer to struct (Lecture 08).

## What I can explain
1. How a ghost chooses: test 4 directions, drop walls, set the reverse aside, pick uniformly with valid[rand() % n].
2. Why DIR_R/DIR_C are two arrays: direction i is (DIR_R[i], DIR_C[i]).
3. How delay works: an eaten ghost waits RESPAWN_DELAY ticks and is neither drawn nor dangerous.

## Questions I may get
- **What if a ghost is in a dead end?** backIdx keeps the reverse direction as the last resort, so it can turn around.
- **How is it uniform?** rand() % n over the n valid directions - each index has (almost) the same chance.

## My own notes (write this yourself)
- What was difficult for me:
- One bug I found or fixed:
- What I would improve:
