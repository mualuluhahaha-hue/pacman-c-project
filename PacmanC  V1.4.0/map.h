/*
 * map.h - board representation: loading, queries and integrity checks.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 1 (Map & Rendering).
 */
#ifndef MAP_H
#define MAP_H

#include "game.h"

#define MAP_ERR_LEN 192

/* A map definition: a name plus up to MAP_MAX_ROWS strings. */
struct MapDef {
    const char *name;
    int rows;
    const char *lines[MAP_MAX_ROWS];
};

/* --- embedded maps (Part 1) ------------------------------------------- */
int  map_embedded_count(void);
const struct MapDef *map_embedded(int index);          /* 0-based */

/* --- loading ----------------------------------------------------------- */
/* Load from a MapDef; fills board, entity start cells and pellet counters.
 * Returns 0 on success, -1 on error (message in err when err != NULL). */
int map_load_def(struct Game *g, const struct MapDef *def, char *err, int errlen);

/* Extension: load a map from a plain text file.  Blank lines are ignored,
 * lines starting with ';' are comments. */
int map_load_file(struct Game *g, const char *path, char *err, int errlen);

/* --- queries ----------------------------------------------------------- */
int  map_in_bounds(const struct Game *g, int r, int c);
int  map_is_wall(const struct Game *g, int r, int c);   /* out of bounds = wall */
char map_get(const struct Game *g, int r, int c);       /* 0 when out of bounds */
void map_set(struct Game *g, int r, int c, char ch);

int  map_count_pellets(const struct Game *g);
int  map_reachable_pellets(const struct Game *g);       /* flood fill from Pac */

/* Full integrity check: returns 0 when the map is playable, otherwise -1.
 * Checks: equal row width, allowed symbols, exactly one 'P', 1..MAX_GHOSTS
 * 'G', at least one pellet and every pellet reachable from Pac-Man. */
int  map_check(const struct Game *g, char *err, int errlen);

#endif /* MAP_H */
