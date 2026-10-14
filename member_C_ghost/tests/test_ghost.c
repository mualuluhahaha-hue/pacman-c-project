/* test_ghost.c - Member C's tests for ghost.c
 * Build: gcc tests/test_ghost.c src/map.c src/player.c src/ghost.c src/engine.c -o test_ghost */
#include <stdio.h>
#include "../src/pacman.h"

int passed = 0;
int failed = 0;

void check(char *name, int cond)
{
    if (cond) { passed++; printf("  [PASS] %s\n", name); }
    else      { failed++; printf("  [FAIL] %s\n", name); }
}

int main(void)
{
    struct Game a, b;
    int i, t, ok, noWall, noJump, noReverse, oldR, oldC, oldDr, oldDc, dist;

    printf("Member C: ghost logic\n");

    game_init(&a, 42, 4);
    noWall = 1;
    noJump = 1;
    noReverse = 1;
    for (t = 0; t < 2000; t++) {
        for (i = 0; i < a.ghostCount; i++) {
            oldR = a.ghosts[i].pos.r;
            oldC = a.ghosts[i].pos.c;
            oldDr = a.ghosts[i].dr;
            oldDc = a.ghosts[i].dc;
            ghost_step(&a, &a.ghosts[i]);
            if (map_is_wall(&a, a.ghosts[i].pos.r, a.ghosts[i].pos.c)) noWall = 0;
            dist = (a.ghosts[i].pos.r - oldR) * (a.ghosts[i].pos.r - oldR)
                 + (a.ghosts[i].pos.c - oldC) * (a.ghosts[i].pos.c - oldC);
            if (dist != 1) noJump = 0;                    /* exactly one cell per step */
            if (a.ghosts[i].dr == -oldDr && a.ghosts[i].dc == -oldDc
                && (oldDr != 0 || oldDc != 0)) noReverse = 0;
        }
    }
    check("ghosts never enter a wall (2000 steps x 4 ghosts)", noWall);
    check("a ghost moves exactly one cell per step", noJump);
    check("a ghost never reverses (the map has no dead ends)", noReverse);

    /* same seed -> same ghost positions */
    game_init(&a, 7, 4);
    for (t = 0; t < 300; t++) ghosts_move_all(&a);
    game_init(&b, 7, 4);
    for (t = 0; t < 300; t++) ghosts_move_all(&b);
    ok = 1;
    for (i = 0; i < 4; i++)
        if (a.ghosts[i].pos.r != b.ghosts[i].pos.r || a.ghosts[i].pos.c != b.ghosts[i].pos.c) ok = 0;
    check("same seed -> identical ghost positions after 300 ticks", ok);

    game_init(&b, 8, 4);
    for (t = 0; t < 300; t++) ghosts_move_all(&b);
    ok = 0;
    for (i = 0; i < 4; i++)
        if (a.ghosts[i].pos.r != b.ghosts[i].pos.r || a.ghosts[i].pos.c != b.ghosts[i].pos.c) ok = 1;
    check("different seed -> different ghost positions", ok);

    /* respawn delay and reset */
    game_init(&a, 1, 4);
    a.ghosts[0].delay = 3;
    ghosts_move_all(&a);
    check("a waiting ghost does not move, it counts down",
          a.ghosts[0].delay == 2 &&
          a.ghosts[0].pos.r == a.ghosts[0].start.r && a.ghosts[0].pos.c == a.ghosts[0].start.c);
    for (t = 0; t < 50; t++) ghosts_move_all(&a);
    ghosts_reset(&a);
    ok = 1;
    for (i = 0; i < 4; i++)
        if (a.ghosts[i].pos.r != a.ghosts[i].start.r || a.ghosts[i].pos.c != a.ghosts[i].start.c
            || a.ghosts[i].delay != 0) ok = 0;
    check("ghosts_reset puts every ghost on its start cell", ok);

    /* ghost_at */
    game_init(&a, 1, 4);
    check("ghost_at finds a ghost on its start cell",
          ghost_at(&a, a.ghosts[2].start.r, a.ghosts[2].start.c) >= 0);
    check("ghost_at returns -1 on an empty cell", ghost_at(&a, 1, 1) == -1);
    a.ghosts[0].delay = 4;
    a.ghosts[1].delay = 4;
    a.ghosts[2].delay = 4;
    a.ghosts[3].delay = 4;
    check("ghost_at ignores ghosts that are waiting",
          ghost_at(&a, a.ghosts[0].start.r, a.ghosts[0].start.c) == -1);

    printf("\nResult: %d passed, %d failed\n", passed, failed);
    return failed;
}
