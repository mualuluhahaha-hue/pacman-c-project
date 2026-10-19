/*
 * entity.c - positions and one-cell movement (see entity.h).
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 2 (Player Control & Rules), shared with Part 3 (Ghost Logic).
 */
#include "entity.h"
#include "map.h"

int pos_equal(struct Pos a, struct Pos b)
{
    return (a.r == b.r) && (a.c == b.c);
}

int dir_is_unit(int dr, int dc)
{
    if (dr == 0 && (dc == 1 || dc == -1)) {
        return 1;
    }
    if (dc == 0 && (dr == 1 || dr == -1)) {
        return 1;
    }
    return 0;
}

int cell_is_open(const struct Game *g, int r, int c)
{
    return map_in_bounds(g, r, c) && !map_is_wall(g, r, c);
}

void entity_face(struct Entity *e, int dr, int dc)
{
    if (dir_is_unit(dr, dc)) {
        e->dr = dr;
        e->dc = dc;
    }
}

int entity_step(struct Game *g, struct Entity *e)
{
    int nr;
    int nc;

    if (!dir_is_unit(e->dr, e->dc)) {
        return 0;
    }

    nr = e->pos.r + e->dr;
    nc = e->pos.c + e->dc;

    if (!cell_is_open(g, nr, nc)) {
        return 0;   /* wall (or board edge): stay in place */
    }

    e->pos.r = nr;
    e->pos.c = nc;
    return 1;
}
