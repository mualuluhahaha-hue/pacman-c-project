/*
 * render.h - ASCII rendering of the board and the status line.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 1 (Map & Rendering).
 *
 * Two display paths exist:
 *
 *   render_all()    - plain text, exactly the output of the first release.
 *                     --plain uses it and the test suite compares its logs.
 *   render_frame()  - the "game" display: one framed picture with a HUD, built
 *                     in a screen buffer and written in one pass, so nothing
 *                     flickers and nothing scrolls.
 */
#ifndef RENDER_H
#define RENDER_H

#include <stdio.h>
#include "game.h"

/* --- plain display (used by --plain and by every recorded test log) ------ */
void render_banner(const struct Game *g, FILE *out);
void render_board(const struct Game *g, FILE *out);
void render_status(const struct Game *g, FILE *out);
void render_all(const struct Game *g, FILE *out);

/* --- framed display ------------------------------------------------------ */
/* Draw the complete game picture (frame, HUD, board, footer) and flush it.
 * color == 0 keeps the picture but drops the colour escapes. */
void render_frame(const struct Game *g, FILE *out, int color);

/* The start screen: title art, subtitle and a short key hint. */
void render_title(FILE *out, int color);

#endif /* RENDER_H */

