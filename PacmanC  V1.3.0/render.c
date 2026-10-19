/*
 * render.c - ASCII rendering of board and status line.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 1 (Map & Rendering).
 *
 * The board is printed one row at a time from top to bottom, exactly as the
 * brief asks ("Keep the display simple (print lines top to bottom)").
 */
#include <stdio.h>
#include "render.h"
#include "map.h"

static char cell_to_char(const struct Game *g, int r, int c)
{
    int i;
    char ch = g->board[r][c];

    if (ch == '\0') {
        ch = CELL_EMPTY;
    }

    /* bonus fruit (extension) */
    if (g->fruitActive && g->fruitPos.r == r && g->fruitPos.c == c) {
        ch = CHAR_FRUIT;
    }

    /* ghosts: normal 'G', scared 'g', eaten ghosts are off the board */
    for (i = 0; i < g->ghostCount; i++) {
        const struct Entity *gh = &g->ghosts[i];

        if (gh->state == GHOST_EATEN) {
            continue;
        }
        if (gh->pos.r == r && gh->pos.c == c) {
            ch = (gh->state == GHOST_SCARED) ? CHAR_SCARED : CHAR_GHOST;
        }
    }

    /* Pac-Man is drawn last so it is always visible */
    if (g->pac.pos.r == r && g->pac.pos.c == c) {
        ch = CHAR_PAC;
    }

    return ch;
}

void render_board(const struct Game *g, FILE *out)
{
    char line[MAP_MAX_COLS + 1];
    int r, c;

    for (r = 0; r < g->rows; r++) {
        for (c = 0; c < g->cols; c++) {
            line[c] = cell_to_char(g, r, c);
        }
        line[g->cols] = '\0';
        fprintf(out, "%s\n", line);
    }
}

void render_status(const struct Game *g, FILE *out)
{
    fprintf(out, "Level %d/%d | Score %d | Lives %d | Pellets %d/%d | Tick %d | Ghosts %d",
            g->level, g->levels, g->score, g->lives,
            g->pelletsRemaining, g->pelletsTotal, g->tick, g->ghostCount);

    if ((g->flags & FEAT_POWER) && g->scaredTicks > 0) {
        fprintf(out, " | SCARED %d", g->scaredTicks);
    }
    if (g->fruitActive) {
        fprintf(out, " | FRUIT %d", g->fruitTicksLeft);
    }
    fprintf(out, "\n");
}

void render_all(const struct Game *g, FILE *out)
{
    render_board(g, out);
    render_status(g, out);
}

void render_banner(const struct Game *g, FILE *out)
{
    fprintf(out, "==========================================================\n");
    fprintf(out, "  PAC-MAN  (text based, standard C only)\n");
    fprintf(out, "  map=%s\n", g->mapName);
    fprintf(out, "  difficulty=%d  seed=%u  ghosts=%d  levels=%d  ai=%s\n",
            g->difficulty, g->rngSeed, g->ghostCount, g->levels,
            (g->aiMode == AI_SMART) ? "smart" :
            (g->aiMode == AI_PURE) ? "pure" : "random");
    fprintf(out, "  keys: w=up  s=down  a=left  d=right  q=quit\n");
    fprintf(out, "  symbols: # wall   . pellet   O power pellet   P you   "
                 "G ghost   %c scared   %c fruit\n", CHAR_SCARED, CHAR_FRUIT);
    fprintf(out, "==========================================================\n");
}
