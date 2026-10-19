/*
 * map.c - board representation: loading, queries and integrity checks.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 1 (Map & Rendering).
 *
 * Board symbols (brief section 1):
 *   '#' wall      '.' pellet      'O' power pellet (extension)
 *   ' ' empty corridor
 *   'P' Pac-Man start cell, 'G' ghost start cell - both are replaced by
 *   empty space in the board array and only kept as entity start positions,
 *   exactly as the brief describes ("positions will be overwritten at
 *   runtime by entities").
 */
#include <stdio.h>
#include <string.h>
#include "map.h"

/* ------------------------------------------------------- embedded maps */
/* The example map of the brief lost its spaces while it was copied into
 * the PDF/Word document (rows 4 and 8 are shorter than the others), so both
 * embedded maps below are complete, equal-width mazes in the same style.
 * Both are validated by the test suite (equal width, all pellets reachable). */
static const struct MapDef EMBEDDED_MAPS[] = {
    {
        "level-1 (24x11, 4 ghost cells, 2 power pellets)",
        11,
        {
            "########################",
            "#..........##..........#",
            "#.####.###.##.###.####.#",
            "#O#  #.#.......#.#  #O.#",
            "#.####.#.#####.#.#####.#",
            "#......#...P...#...... #",
            "#.####.#.#####.#.#####.#",
            "#.#  #.# GG  GG #.#  #.#",
            "#.####.###.##.###.####.#",
            "#..........##..........#",
            "########################"
        }
    },
    {
        "level-2 (rotated 24x11 maze, denser feel, 4 ghost cells)",
        24,
        {
            "###########",
            "#..O......#",
            "#.###.###.#",
            "#.# #.# #.#",
            "#.# #.# #.#",
            "#.###.###.#",
            "#.........#",
            "#.#######.#",
            "#.#.... #.#",
            "#.#.#.#G#.#",
            "#...#.#G..#",
            "###.#P# ###",
            "###.#.# ###",
            "#...#.#G..#",
            "#.#....G#.#",
            "#.##### #.#",
            "#.#....##.#",
            "#..##.#...#",
            "#.# #.###.#",
            "#.# #.# #.#",
            "#.###.# #.#",
            "#.#O#.###.#",
            "#.... ....#",
            "###########"
        }
    },
    {
        "level-3 (24x13 ladder with loops, 4 ghost cells)",
        13,
        {
            "########################",
            "#P.....................#",
            "#.###.#####.####.####.##",
            "#......................#",
            "####.####.####.####.##.#",
            "#......................#",
            "#.#.####.######.####.###",
            "#...GG............GG...#",
            "#####.######.#####.##..#",
            "#......................#",
            "#.##.#####.#####.####.##",
            "#......................#",
            "########################"
        }
    },
    {
        "level-4 (28x13 concentric rings, 4 ghost cells)",
        13,
        {
            "############################",
            "#P.........................#",
            "#.###########.############.#",
            "#.#......................#.#",
            "#.#.####################.#.#",
            "#.#.#...GG........GG...#.#.#",
            "#.#.#.#######.########.#.#.#",
            "#.#.#..................#.#.#",
            "#.#.#########.##########.#.#",
            "#.#......................#.#",
            "#.########################.#",
            "#..........................#",
            "############################"
        }
    },
    {
        "level-5 (24x13 lattice, 4 ghost cells)",
        13,
        {
            "########################",
            "#..........GG..........#",
            "#.#.#.#.#.#.#.#.#.#.#.##",
            "#......................#",
            "#.#.#.#.#.#.#.#.#.#.#.##",
            "#......................#",
            "#.#.#.#.#.#GG.#.#.#.#.##",
            "#......................#",
            "#.#.#.#.#.#.#.#.#.#.#.##",
            "#......................#",
            "#.#.#.#.#.#.#.#.#.#.#.##",
            "#P.....................#",
            "########################"
        }
    }
};

int map_embedded_count(void)
{
    return (int)(sizeof EMBEDDED_MAPS / sizeof EMBEDDED_MAPS[0]);
}

const struct MapDef *map_embedded(int index)
{
    if (index < 0 || index >= map_embedded_count()) {
        return NULL;
    }
    return &EMBEDDED_MAPS[index];
}

/* ------------------------------------------------------------ queries */
int map_in_bounds(const struct Game *g, int r, int c)
{
    return (r >= 0) && (r < g->rows) && (c >= 0) && (c < g->cols);
}

int map_is_wall(const struct Game *g, int r, int c)
{
    if (!map_in_bounds(g, r, c)) {
        return 1;                    /* the board edge behaves like a wall */
    }
    return g->board[r][c] == CELL_WALL;
}

char map_get(const struct Game *g, int r, int c)
{
    if (!map_in_bounds(g, r, c)) {
        return '\0';
    }
    return g->board[r][c];
}

void map_set(struct Game *g, int r, int c, char ch)
{
    if (map_in_bounds(g, r, c)) {
        g->board[r][c] = ch;
    }
}

int map_count_pellets(const struct Game *g)
{
    int r, c, n = 0;

    for (r = 0; r < g->rows; r++) {
        for (c = 0; c < g->cols; c++) {
            char ch = g->board[r][c];
            if (ch == CELL_PELLET || ch == CELL_POWER) {
                n++;
            }
        }
    }
    return n;
}

/* Flood fill from Pac-Man's start cell; counts the pellets that can be
 * reached.  Used by the "--check-map" option and by the unit tests, so a
 * map with an unreachable pellet can never reach the player by accident. */
int map_reachable_pellets(const struct Game *g)
{
    char seen[MAP_MAX_ROWS][MAP_MAX_COLS];
    int stack_r[MAP_MAX_ROWS * MAP_MAX_COLS];
    int stack_c[MAP_MAX_ROWS * MAP_MAX_COLS];
    static const int DR[4] = { -1, 1, 0, 0 };
    static const int DC[4] = { 0, 0, -1, 1 };
    int top = 0;
    int count = 0;
    int r, c, i;

    memset(seen, 0, sizeof seen);

    r = g->pac.start.r;
    c = g->pac.start.c;
    if (!map_in_bounds(g, r, c) || map_is_wall(g, r, c)) {
        return 0;
    }

    seen[r][c] = 1;
    stack_r[top] = r;
    stack_c[top] = c;
    top++;

    while (top > 0) {
        top--;
        r = stack_r[top];
        c = stack_c[top];

        if (g->board[r][c] == CELL_PELLET || g->board[r][c] == CELL_POWER) {
            count++;
        }

        for (i = 0; i < 4; i++) {
            int nr = r + DR[i];
            int nc = c + DC[i];
            if (map_in_bounds(g, nr, nc) && !map_is_wall(g, nr, nc) && !seen[nr][nc]) {
                seen[nr][nc] = 1;
                stack_r[top] = nr;
                stack_c[top] = nc;
                top++;
            }
        }
    }

    return count;
}

/* ------------------------------------------------------------ loading */
static int is_allowed(char ch)
{
    return ch == CELL_WALL || ch == CELL_PELLET || ch == CELL_POWER ||
           ch == CELL_EMPTY || ch == CHAR_PAC || ch == CHAR_GHOST;
}

static void set_err(char *err, int errlen, const char *msg)
{
    if (err != NULL && errlen > 0) {
        snprintf(err, (size_t)errlen, "%s", msg);
    }
}

static int load_lines(struct Game *g, const char *name, const char *const *lines,
                      int n, int isFile, char *err, int errlen)
{
    char msg[MAP_ERR_LEN];
    int width = -1;
    int pacCount = 0;
    int ghostCount = 0;
    int r, c;

    if (n < 3) {
        set_err(err, errlen, "map needs at least 3 rows");
        return -1;
    }
    if (n > MAP_MAX_ROWS) {
        set_err(err, errlen, "map has more rows than MAP_MAX_ROWS");
        return -1;
    }

    for (r = 0; r < n; r++) {
        int len = (int)strlen(lines[r]);
        if (width < 0) {
            width = len;
        } else if (len != width) {
            snprintf(msg, sizeof msg,
                     "row %d is %d characters wide but row 0 is %d "
                     "(trailing spaces are significant)", r, len, width);
            set_err(err, errlen, msg);
            return -1;
        }
    }
    if (width < 3) {
        set_err(err, errlen, "map is too narrow");
        return -1;
    }
    if (width > MAP_MAX_COLS) {
        set_err(err, errlen, "map is wider than MAP_MAX_COLS (40)");
        return -1;
    }

    /* start from a clean board */
    for (r = 0; r < MAP_MAX_ROWS; r++) {
        for (c = 0; c < MAP_MAX_COLS; c++) {
            g->board[r][c] = CELL_EMPTY;
        }
    }
    for (r = 0; r < MAX_GHOSTS; r++) {
        g->ghosts[r].pos.r = g->ghosts[r].pos.c = 0;
        g->ghosts[r].start.r = g->ghosts[r].start.c = 0;
        g->ghosts[r].dr = g->ghosts[r].dc = 0;
        g->ghosts[r].state = GHOST_NORMAL;
        g->ghosts[r].respawnIn = 0;
        g->ghosts[r].followTicks = 0;
        g->ghosts[r].followCooldown = 0;
        g->ghosts[r].forceReverse = 0;
    }

    for (r = 0; r < n; r++) {
        for (c = 0; c < width; c++) {
            char ch = lines[r][c];

            if (!is_allowed(ch)) {
                snprintf(msg, sizeof msg,
                         "illegal character '%c' (0x%02X) at row %d column %d",
                         (ch >= 32 && ch < 127) ? ch : '?',
                         (unsigned char)ch, r, c);
                set_err(err, errlen, msg);
                return -1;
            }

            if (ch == CHAR_PAC) {
                pacCount++;
                g->pac.start.r = r;
                g->pac.start.c = c;
                ch = CELL_EMPTY;
            } else if (ch == CHAR_GHOST) {
                if (ghostCount < MAX_GHOSTS) {
                    g->ghosts[ghostCount].start.r = r;
                    g->ghosts[ghostCount].start.c = c;
                }
                ghostCount++;
                ch = CELL_EMPTY;
            }

            g->board[r][c] = ch;
        }
    }

    if (pacCount != 1) {
        set_err(err, errlen, "map must contain exactly one 'P' start cell");
        return -1;
    }
    if (ghostCount < 1) {
        set_err(err, errlen, "map must contain at least one 'G' start cell");
        return -1;
    }
    if (ghostCount > MAX_GHOSTS) {
        set_err(err, errlen, "map contains more than 4 'G' start cells");
        return -1;
    }

    g->rows = n;
    g->cols = width;
    g->ghostCount = ghostCount;

    g->pac.pos = g->pac.start;
    g->pac.dr = 0;
    g->pac.dc = 0;

    for (r = 0; r < ghostCount; r++) {
        g->ghosts[r].pos = g->ghosts[r].start;
        g->ghosts[r].dr = 0;
        g->ghosts[r].dc = 0;
        g->ghosts[r].state = GHOST_NORMAL;
        g->ghosts[r].respawnIn = 0;
    }

    g->pelletsTotal = map_count_pellets(g);
    g->pelletsRemaining = g->pelletsTotal;

    if (g->pelletsTotal < 1) {
        set_err(err, errlen, "map must contain at least one pellet");
        return -1;
    }

    snprintf(g->mapName, sizeof g->mapName, "%s", name);
    g->mapIsFile = isFile;

    return 0;
}

int map_load_def(struct Game *g, const struct MapDef *def, char *err, int errlen)
{
    if (def == NULL) {
        set_err(err, errlen, "no map definition");
        return -1;
    }
    return load_lines(g, def->name, def->lines, def->rows, 0, err, errlen);
}

int map_load_file(struct Game *g, const char *path, char *err, int errlen)
{
    char storage[MAP_MAX_ROWS][MAP_MAX_COLS + 8];
    const char *lines[MAP_MAX_ROWS];
    char buf[MAP_MAX_COLS + 32];
    char msg[MAP_ERR_LEN];
    FILE *fp;
    int n = 0;

    fp = fopen(path, "r");
    if (fp == NULL) {
        snprintf(msg, sizeof msg, "cannot open map file '%s'", path);
        set_err(err, errlen, msg);
        return -1;
    }

    while (fgets(buf, (int)sizeof buf, fp) != NULL) {
        size_t len = strlen(buf);

        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }
        if (len == 0) {
            continue;                      /* blank line */
        }
        if (buf[0] == ';') {
            continue;                      /* comment    */
        }
        if (n >= MAP_MAX_ROWS) {
            fclose(fp);
            set_err(err, errlen, "map file has more rows than MAP_MAX_ROWS");
            return -1;
        }
        if (len > MAP_MAX_COLS) {
            fclose(fp);
            snprintf(msg, sizeof msg,
                     "map file line %d is wider than MAP_MAX_COLS (40)", n + 1);
            set_err(err, errlen, msg);
            return -1;
        }
        memcpy(storage[n], buf, len + 1);
        lines[n] = storage[n];
        n++;
    }
    fclose(fp);

    return load_lines(g, path, lines, n, 1, err, errlen);
}

/* -------------------------------------------------------- integrity */
int map_check(const struct Game *g, char *err, int errlen)
{
    char msg[MAP_ERR_LEN];
    int r, c;
    int pellets;
    int reachable;

    if (g->rows < 3 || g->cols < 3 || g->rows > MAP_MAX_ROWS || g->cols > MAP_MAX_COLS) {
        set_err(err, errlen, "board dimensions out of range");
        return -1;
    }

    for (r = 0; r < g->rows; r++) {
        for (c = 0; c < g->cols; c++) {
            if (!is_allowed(g->board[r][c])) {
                set_err(err, errlen, "board contains an illegal cell value");
                return -1;
            }
        }
    }

    if (map_is_wall(g, g->pac.start.r, g->pac.start.c)) {
        set_err(err, errlen, "Pac-Man starts inside a wall");
        return -1;
    }

    pellets = map_count_pellets(g);
    if (pellets < 1) {
        set_err(err, errlen, "no pellets on the board");
        return -1;
    }

    reachable = map_reachable_pellets(g);
    if (reachable != pellets) {
        snprintf(msg, sizeof msg,
                 "%d of %d pellets cannot be reached from Pac-Man's start cell",
                 pellets - reachable, pellets);
        set_err(err, errlen, msg);
        return -1;
    }

    return 0;
}
