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
#include <string.h>
#include "render.h"
#include "map.h"
#include "screen.h"

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

/* ======================================================================
 * Framed display
 * ====================================================================== */

/* Character and colour of one board cell, with Pac-Man drawn on top and the
 * ghosts above the pellets. */
static char board_cell(const struct Game *g, int r, int c, int *color)
{
    int i;
    char ch = g->board[r][c];

    *color = C_NONE;
    if (ch == CELL_WALL) {
        *color = C_WALL;
    } else if (ch == CELL_PELLET) {
        *color = C_PELLET;
    } else if (ch == CELL_POWER) {
        *color = C_POWER;
    } else {
        ch = CELL_EMPTY;
    }

    if (g->fruitActive && g->fruitPos.r == r && g->fruitPos.c == c) {
        ch = CHAR_FRUIT;
        *color = C_FRUIT;
    }

    for (i = 0; i < g->ghostCount; i++) {
        const struct Entity *gh = &g->ghosts[i];

        if (gh->state == GHOST_EATEN) {
            continue;
        }
        if (gh->pos.r == r && gh->pos.c == c) {
            ch = (gh->state == GHOST_SCARED) ? CHAR_SCARED : CHAR_GHOST;
            *color = (gh->state == GHOST_SCARED) ? C_SCARED : C_GHOST;
        }
    }

    if (g->pac.pos.r == r && g->pac.pos.c == c) {
        ch = CHAR_PAC;
        *color = C_PAC;
    }

    return ch;
}

/* A horizontal rule inside the frame, with an optional title in the middle. */
static void internal_rule(struct Screen *s, int row, const char *title, int color)
{
    int c;

    for (c = 1; c < s->cols - 1; c++) {
        s->ch[row][c] = '-';
        s->color[row][c] = (unsigned char)color;
    }
    if (title != NULL) {
        int width = (int)strlen(title);
        int start = 2;

        if (width > s->cols - 6) {
            width = s->cols - 6;
        }
        screen_put(s, row, start, " ", C_NONE);
        screen_put(s, row, start + 1, title, C_HUD);
        if (start + 1 + width + 1 < s->cols - 1) {
            screen_put(s, row, start + 1 + width + 1, " ", C_NONE);
        }
    }
}

void render_frame(const struct Game *g, FILE *out, int color)
{
    struct Screen s;
    char hud[SCREEN_MAX_COLS];
    const char *footer;
    int box_w;
    int rows;
    int board_offset;
    int r, c;

    if (out == NULL) {
        return;
    }

    /* the frame is at least wide enough for the HUD line */
    box_w = g->cols + 4;
    if (box_w < 76) {
        box_w = 76;
    }
    if (box_w > SCREEN_MAX_COLS - 1) {
        box_w = SCREEN_MAX_COLS - 1;
    }
    rows = g->rows + 6;

    screen_init(&s, rows, box_w);
    screen_box(&s, 0, 0, rows - 1, box_w - 1, C_BORDER);

    /* ---- HUD line ------------------------------------------------------ */
    if ((g->flags & FEAT_POWER) && g->scaredTicks > 0) {
        snprintf(hud, sizeof hud,
                 " LEVEL %d/%d   SCORE %05d   LIVES %d   PELLETS %d/%d   "
                 "TICK %d   SCARED %d",
                 g->level, g->levels, g->score, g->lives,
                 g->pelletsRemaining, g->pelletsTotal, g->tick, g->scaredTicks);
    } else if (g->fruitActive) {
        snprintf(hud, sizeof hud,
                 " LEVEL %d/%d   SCORE %05d   LIVES %d   PELLETS %d/%d   "
                 "TICK %d   FRUIT %d",
                 g->level, g->levels, g->score, g->lives,
                 g->pelletsRemaining, g->pelletsTotal, g->tick,
                 g->fruitTicksLeft);
    } else {
        snprintf(hud, sizeof hud,
                 " LEVEL %d/%d   SCORE %05d   LIVES %d   PELLETS %d/%d   TICK %d",
                 g->level, g->levels, g->score, g->lives,
                 g->pelletsRemaining, g->pelletsTotal, g->tick);
    }
    screen_put(&s, 1, 1, hud, C_HUD);

    /* rule under the HUD */
    internal_rule(&s, 2, NULL, C_BORDER);

    /* the name of the game sits in the top border */
    screen_put(&s, 0, 2, " PAC-MAN ", C_TITLE);
    {
        char tag[64];

        snprintf(tag, sizeof tag, " level %d ", g->level);
        screen_put(&s, 0, box_w - 1 - (int)strlen(tag), tag, C_HUD);
    }

    /* ---- board --------------------------------------------------------- */
    board_offset = 2 + (box_w - 4 - g->cols) / 2;
    if (board_offset < 2) {
        board_offset = 2;
    }
    for (r = 0; r < g->rows; r++) {
        for (c = 0; c < g->cols; c++) {
            int cell_color = C_NONE;
            char ch = board_cell(g, r, c, &cell_color);

            s.ch[3 + r][board_offset + c] = ch;
            s.color[3 + r][board_offset + c] = (unsigned char)cell_color;
        }
    }

    /* ---- footer -------------------------------------------------------- */
    footer = (g->lastEvent[0] != '\0') ? g->lastEvent
                                       : "W/A/S/D move   Q quit";
    internal_rule(&s, rows - 3, NULL, C_BORDER);
    {
        char line[SCREEN_MAX_COLS];

        snprintf(line, sizeof line, " %s", footer);
        screen_put(&s, rows - 2, 1, line, C_HUD);
    }

    screen_flush(&s, out, color);
}

void render_title(FILE *out, int color)
{
    static const char *const ART[] = {
        "  ____   _    ____      __  __    _    _   _ ",
        " |  _ \\ / \\  / ___|    |  \\/  |  / \\  | \\ | |",
        " | |_) / _ \\| |        | |\\/| | / _ \\ |  \\| |",
        " |  __/ ___ \\ |___     | |  | |/ ___ \\| |\\  |",
        " |_|  /_/   \\_\\____|    |_|  |_/_/   \\_\\_| \\_|"
    };
    enum { ART_LINES = 5 };
    struct Screen s;
    int rows = 15;
    int cols = 58;
    int i;

    if (out == NULL) {
        return;
    }

    screen_init(&s, rows, cols);
    screen_box(&s, 0, 0, rows - 1, cols - 1, C_BORDER);

    for (i = 0; i < ART_LINES; i++) {
        screen_put(&s, 3 + i, 4, ART[i], C_TITLE);
    }

    screen_put(&s, 9, 4, "text based Pac-Man - standard C only", C_HUD);
    screen_put(&s, 10, 4, "MFE113TC group project - Group 6", C_HUD);
    internal_rule(&s, 11, NULL, C_BORDER);
    screen_put(&s, 12, 4, "[1]-[5] choose a difficulty    [Q] quit", C_GOOD);
    screen_put(&s, 13, 4, "each tick: one move, then the ghosts move", C_HUD);

    screen_flush(&s, out, color);
}
