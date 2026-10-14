/* ghost.c - Member C: Ghost Logic */
#include <stdlib.h>
#include "pacman.h"

const int DIR_R[4] = { -1, 1, 0, 0 };      /* up, down, left, right */
const int DIR_C[4] = { 0, 0, -1, 1 };

/* Move one ghost one step: choose uniformly among the valid directions.
 * The reverse of the last direction is used only if nothing else is valid. */
void ghost_step(struct Game *g, struct Entity *e)
{
    int valid[4];
    int n, backIdx, i, nr, nc;

    n = 0;
    backIdx = -1;
    for (i = 0; i < 4; i++) {
        nr = e->pos.r + DIR_R[i];
        nc = e->pos.c + DIR_C[i];
        if (map_is_wall(g, nr, nc))
            continue;                              /* wall: not a valid move */
        if (DIR_R[i] == -e->dr && DIR_C[i] == -e->dc) {
            backIdx = i;                           /* reversing: last resort */
            continue;
        }
        valid[n] = i;
        n++;
    }
    if (n == 0 && backIdx >= 0) {
        valid[0] = backIdx;
        n = 1;
    }
    if (n == 0)
        return;                                    /* boxed in: stay */

    i = valid[rand() % n];                         /* uniform random choice */
    e->pos.r = e->pos.r + DIR_R[i];
    e->pos.c = e->pos.c + DIR_C[i];
    e->dr = DIR_R[i];
    e->dc = DIR_C[i];
}

/* Every ghost moves once; a ghost that waits to respawn only counts down. */
void ghosts_move_all(struct Game *g)
{
    int i;

    for (i = 0; i < g->ghostCount; i++) {
        if (g->ghosts[i].delay > 0)
            g->ghosts[i].delay--;
        else
            ghost_step(g, &g->ghosts[i]);
    }
}

/* All ghosts go back to their start cells (after Pac-Man loses a life). */
void ghosts_reset(struct Game *g)
{
    int i;

    for (i = 0; i < g->ghostCount; i++) {
        g->ghosts[i].pos.r = g->ghosts[i].start.r;
        g->ghosts[i].pos.c = g->ghosts[i].start.c;
        g->ghosts[i].dr = 0;
        g->ghosts[i].dc = 0;
        g->ghosts[i].delay = 0;
    }
}

/* Index of an active ghost on cell (r,c), or -1 if there is none. */
int ghost_at(struct Game *g, int r, int c)
{
    int i;

    for (i = 0; i < g->ghostCount; i++)
        if (g->ghosts[i].delay == 0 &&
            g->ghosts[i].pos.r == r && g->ghosts[i].pos.c == c)
            return i;
    return -1;
}
