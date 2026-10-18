/*
 * screen.c - off-screen frame buffer and 16 colour terminal output.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 1 (Map & Rendering).
 */
#include <string.h>
#include "screen.h"

/* ANSI SGR codes for the colour slots.  Bright variants (9x) keep the maze
 * readable on a dark terminal, which is the default in every lab. */
static const char *const ANSI[C_COUNT] = {
    "",             /* C_NONE   */
    "\x1b[36m",     /* C_BORDER cyan        */
    "\x1b[94m",     /* C_WALL   bright blue */
    "\x1b[37m",     /* C_PELLET white       */
    "\x1b[93m",     /* C_POWER  bright yellow */
    "\x1b[1;93m",   /* C_PAC    bold yellow */
    "\x1b[91m",     /* C_GHOST  bright red  */
    "\x1b[96m",     /* C_SCARED bright cyan */
    "\x1b[95m",     /* C_FRUIT  magenta     */
    "\x1b[97m",     /* C_HUD    bright white */
    "\x1b[1;93m",   /* C_TITLE  bold yellow */
    "\x1b[91m",     /* C_WARN   red         */
    "\x1b[92m"      /* C_GOOD   green       */
};

#define ANSI_RESET "\x1b[0m"

static int clamp(int value, int lo, int hi)
{
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

void screen_init(struct Screen *s, int rows, int cols)
{
    s->rows = clamp(rows, 1, SCREEN_MAX_ROWS);
    s->cols = clamp(cols, 1, SCREEN_MAX_COLS);
    screen_clear(s);
}

void screen_clear(struct Screen *s)
{
    int r, c;

    for (r = 0; r < s->rows; r++) {
        for (c = 0; c < s->cols; c++) {
            s->ch[r][c] = ' ';
            s->color[r][c] = C_NONE;
        }
    }
}

void screen_put(struct Screen *s, int row, int col, const char *text, int color)
{
    int c = col;

    if (row < 0 || row >= s->rows) {
        return;
    }
    for (; *text != '\0' && c < s->cols; text++, c++) {
        if (c < 0) {
            continue;
        }
        s->ch[row][c] = *text;
        s->color[row][c] = (unsigned char)color;
    }
}

void screen_fill_row(struct Screen *s, int row, char ch, int color)
{
    int c;

    if (row < 0 || row >= s->rows) {
        return;
    }
    for (c = 0; c < s->cols; c++) {
        s->ch[row][c] = ch;
        s->color[row][c] = (unsigned char)color;
    }
}

void screen_box(struct Screen *s, int r0, int c0, int r1, int c1, int color)
{
    int c, r;

    if (r0 < 0 || c0 < 0 || r1 >= s->rows || c1 >= s->cols || r1 < r0 || c1 < c0) {
        return;
    }
    for (c = c0; c <= c1; c++) {
        s->ch[r0][c] = '-';
        s->color[r0][c] = (unsigned char)color;
        s->ch[r1][c] = '-';
        s->color[r1][c] = (unsigned char)color;
    }
    for (r = r0; r <= r1; r++) {
        s->ch[r][c0] = '|';
        s->color[r][c0] = (unsigned char)color;
        s->ch[r][c1] = '|';
        s->color[r][c1] = (unsigned char)color;
    }
    s->ch[r0][c0] = '+';
    s->ch[r0][c1] = '+';
    s->ch[r1][c0] = '+';
    s->ch[r1][c1] = '+';
}

void screen_flush(const struct Screen *s, FILE *out, int color)
{
    int r, c;
    int current = -1;

    if (out == NULL) {
        return;
    }

    if (color) {
        fputs("\x1b[H", out);       /* cursor home: overwrite, never scroll */
    }

    for (r = 0; r < s->rows; r++) {
        if (color) {
            current = -1;
        }
        for (c = 0; c < s->cols; c++) {
            char ch = s->ch[r][c];

            if (color) {
                int want = s->color[r][c];

                if (want != current) {
                    if (current != -1 || want != C_NONE) {
                        fputs(ANSI_RESET, out);
                        fputs(ANSI[want], out);
                    }
                    current = want;
                }
            }
            fputc(ch, out);
        }
        if (color) {
            fputs(ANSI_RESET, out);
            fputs("\x1b[K", out);   /* wipe whatever was longer before */
        }
        fputc('\n', out);
    }
    fflush(out);
}

void screen_hide_cursor(FILE *out)
{
    if (out != NULL) {
        fputs("\x1b[?25l", out);
        fflush(out);
    }
}

void screen_clear_terminal(FILE *out)
{
    if (out != NULL) {
        fputs("\x1b[2J", out);
        fflush(out);
    }
}

void screen_reset(FILE *out)
{
    if (out != NULL) {
        fputs(ANSI_RESET, out);
        fputs("\x1b[?25h", out);    /* show the cursor again */
        fflush(out);
    }
}
