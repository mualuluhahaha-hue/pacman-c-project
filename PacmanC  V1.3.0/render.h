/*
 * render.h - ASCII rendering of the board and the status line.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 1 (Map & Rendering).
 */
#ifndef RENDER_H
#define RENDER_H

#include <stdio.h>
#include "game.h"

void render_banner(const struct Game *g, FILE *out);
void render_board(const struct Game *g, FILE *out);
void render_status(const struct Game *g, FILE *out);
void render_all(const struct Game *g, FILE *out);

#endif /* RENDER_H */
