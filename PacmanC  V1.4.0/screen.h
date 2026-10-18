/*
 * screen.h - off-screen frame buffer and 16 colour terminal output.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 1 (Map & Rendering).
 *
 * The whole picture is composed in memory and written to the terminal in one
 * pass, starting with "cursor home" instead of clearing the screen.  That is
 * what removes the flicker of a naive print-every-line renderer.
 *
 * The colours are plain ANSI escape sequences: the program still writes bytes
 * to stdout, so no graphics library is involved and the brief's "ASCII
 * characters, standard C only" requirement is untouched.  --plain switches
 * the whole thing off.
 */
#ifndef SCREEN_H
#define SCREEN_H

#include <stdio.h>

#define SCREEN_MAX_ROWS 40
#define SCREEN_MAX_COLS 120

/* Colour slots - mapped to ANSI codes in screen.c. */
enum ScreenColor {
    C_NONE = 0,
    C_BORDER,
    C_WALL,
    C_PELLET,
    C_POWER,
    C_PAC,
    C_GHOST,
    C_SCARED,
    C_FRUIT,
    C_HUD,
    C_TITLE,
    C_WARN,
    C_GOOD,
    C_COUNT
};

struct Screen {
    int rows;
    int cols;
    char ch[SCREEN_MAX_ROWS][SCREEN_MAX_COLS];
    unsigned char color[SCREEN_MAX_ROWS][SCREEN_MAX_COLS];
};

/* Prepare a frame of the given size (clamped to the maximum). */
void screen_init(struct Screen *s, int rows, int cols);

/* Fill every cell with a space in the default colour. */
void screen_clear(struct Screen *s);

/* Write a NUL terminated string at (row, col); clipped at the frame edge. */
void screen_put(struct Screen *s, int row, int col, const char *text, int color);

/* Draw a single character frame border from (r0,c0) to (r1,c1) inclusive. */
void screen_box(struct Screen *s, int r0, int c0, int r1, int c1, int color);

/* Fill one row with 'ch' in the given colour. */
void screen_fill_row(struct Screen *s, int row, char ch, int color);

/* Emit the frame: cursor home, then every row.  color == 0 writes plain
 * ASCII without escape sequences (used by --plain and by the tests). */
void screen_flush(const struct Screen *s, FILE *out, int color);

/* Terminal control helpers (ANSI). */
void screen_hide_cursor(FILE *out);
void screen_clear_terminal(FILE *out);
void screen_reset(FILE *out);

#endif /* SCREEN_H */
