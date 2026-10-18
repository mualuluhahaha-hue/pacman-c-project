/*
 * ghost.h - ghost behaviour: random walk plus the optional smart modes.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 3 (Ghost Logic).
 */
#ifndef GHOST_H
#define GHOST_H

#include "game.h"

/* Advance the chase/scatter timer (extension). */
void ghost_update_global_mode(struct Game *g);

/* Run one ghost for one tick: respawn countdown, direction choice, movement. */
void ghost_update(struct Game *g, int index);

/* Put every ghost back on its start cell and clear its state. */
void ghost_reset_all(struct Game *g);

/* Number of directions a ghost could legally take from its current cell. */
int ghost_valid_dir_count(const struct Game *g, const struct Entity *gh);

#endif /* GHOST_H */
