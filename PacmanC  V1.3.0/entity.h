/*
 * entity.h - positions and one-cell movement of Pac-Man and the ghosts.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 2 (Player Control & Rules), shared with Part 3 (Ghost Logic).
 */
#ifndef ENTITY_H
#define ENTITY_H

#include "game.h"

int  pos_equal(struct Pos a, struct Pos b);
int  dir_is_unit(int dr, int dc);              /* exactly one of dr/dc is +-1 */
void entity_face(struct Entity *e, int dr, int dc);

/* Move one cell along the current facing.  Returns 1 when the entity really
 * moved, 0 when the target cell is a wall (the entity stays in place). */
int  entity_step(struct Game *g, struct Entity *e);

/* Non-zero when the cell can be entered (inside the board and not a wall). */
int  cell_is_open(const struct Game *g, int r, int c);

#endif /* ENTITY_H */
