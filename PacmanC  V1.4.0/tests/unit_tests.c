/*
 * unit_tests.c - unit test suite for the Pac-Man project.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing) owns the harness and the game-state cases;
 * Part 1, 2 and 3 contributed the cases for their own module.
 *
 * Build and run:
 *     gcc -std=c99 -Wall -Wextra -o unit_tests tests/unit_tests.c rng.c entity.c \
 *         map.c game.c ghost.c render.c input.c
 *     ./unit_tests
 * or simply "make test" / "tests\run_tests.bat".
 *
 * Exit code 0 = every check passed, 1 = at least one check failed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rng.h"
#include "map.h"
#include "entity.h"
#include "game.h"
#include "ghost.h"
#include "input.h"
#include "render.h"
#include "screen.h"

/* ------------------------------------------------------------- harness */
static int checks_run = 0;
static int checks_failed = 0;
static const char *current_suite = "?";

static void check_impl(int ok, const char *what, int line)
{
    checks_run++;
    if (!ok) {
        checks_failed++;
        printf("    FAIL  [%s line %d] %s\n", current_suite, line, what);
    }
}

static void check_int_impl(long got, long want, const char *what, int line)
{
    checks_run++;
    if (got != want) {
        checks_failed++;
        printf("    FAIL  [%s line %d] %s: got %ld, expected %ld\n",
               current_suite, line, what, got, want);
    }
}

static void check_str_impl(const char *got, const char *want, const char *what, int line)
{
    checks_run++;
    if (got == NULL || strcmp(got, want) != 0) {
        checks_failed++;
        printf("    FAIL  [%s line %d] %s: got '%s', expected '%s'\n",
               current_suite, line, what, got ? got : "(null)", want);
    }
}

#define SUITE(name)  do { current_suite = (name); printf("  %s\n", current_suite); } while (0)
#define CHECK(cond)             check_impl((cond) ? 1 : 0, #cond, __LINE__)
#define CHECK_INT(got, want)    check_int_impl((long)(got), (long)(want), #got, __LINE__)
#define CHECK_STR(got, want)    check_str_impl((got), (want), #got, __LINE__)

/* ------------------------------------------------------------- helpers */
/* Build a game from map rows given inline in the test file. */
static int load_inline(struct Game *g, const char **rows, int n, int ghosts,
                       int levels, int flags, int ai)
{
    struct MapDef def;
    char err[MAP_ERR_LEN];
    int i;

    memset(&def, 0, sizeof def);
    def.name = "inline-test-map";
    def.rows = n;
    for (i = 0; i < n; i++) {
        def.lines[i] = rows[i];
    }

    game_init(g, 1u, ghosts, levels, flags, ai);
    if (map_load_def(g, &def, err, (int)sizeof err) != 0) {
        printf("    (map rejected: %s)\n", err);
        return -1;
    }
    g->level = 1;
    g->verbose = 0;
    return 0;
}

static int load_file_map(struct Game *g, const char *path, int ghosts, int levels,
                         int flags, int ai)
{
    char err[MAP_ERR_LEN];

    game_init(g, 1u, ghosts, levels, flags, ai);
    if (map_load_file(g, path, err, (int)sizeof err) != 0) {
        printf("    (map file rejected: %s)\n", err);
        return -1;
    }
    g->level = 1;
    g->verbose = 0;
    return 0;
}

/* Initialise a game and load one of the embedded levels (always call this
 * instead of game_load_level on an uninitialised struct Game). */
static int new_embedded_game(struct Game *g, int level, int ghosts, int levels,
                             int flags, int ai)
{
    game_init(g, 1u, ghosts, levels, flags, ai);
    g->verbose = 0;
    return game_load_level(g, level);
}

/* Move a file map through map_check, used by several cases. */
static int file_map_is_valid(const char *path)
{
    struct Game g;
    char err[MAP_ERR_LEN];

    if (load_file_map(&g, path, 1, 1, FEAT_ALL, AI_RANDOM) != 0) {
        return 0;
    }
    return map_check(&g, err, (int)sizeof err) == 0;
}

/* ---------------------------------------------------- Part 4: RNG ---- */
static void test_rng(void)
{
    struct Rng a;
    uint32_t first[6];
    int i;
    int buckets[6];

    SUITE("Part 4 / rng: reproducibility");

    rng_seed(&a, 1u);
    for (i = 0; i < 6; i++) {
        first[i] = rng_next(&a);
    }

    /* identical seeds must give identical sequences */
    {
        struct Rng x, y;
        int same = 1;
        rng_seed(&x, 12345u);
        rng_seed(&y, 12345u);
        for (i = 0; i < 100; i++) {
            if (rng_next(&x) != rng_next(&y)) {
                same = 0;
            }
        }
        CHECK(same);
    }

    /* golden values: they catch any accidental change of the algorithm,
     * which would silently break every recorded game log */
    CHECK_INT(first[0], 270369L);
    CHECK_INT(first[1], 67634689L);
    CHECK_INT(first[2], 2647435461L);
    CHECK_INT(first[3], 307599695L);

    rng_seed(&a, 42u);
    CHECK_INT(rng_next(&a), 11355432L);

    /* seed 0 must not degenerate into an all-zero state */
    rng_seed(&a, 0u);
    CHECK(rng_next(&a) != 0u);

    /* different seeds must give different sequences */
    {
        struct Rng x, y;
        int diff = 0;
        rng_seed(&x, 7u);
        rng_seed(&y, 8u);
        for (i = 0; i < 20; i++) {
            if (rng_next(&x) != rng_next(&y)) {
                diff = 1;
            }
        }
        CHECK(diff);
    }

    SUITE("Part 4 / rng: range behaviour");
    rng_seed(&a, 42u);
    CHECK_INT(rng_range(&a, 0u), 0L);
    CHECK_INT(rng_range(&a, 1u), 0L);

    rng_seed(&a, 99u);
    for (i = 0; i < 10000; i++) {
        if ((int)rng_range(&a, 6u) >= 6) {
            CHECK(0);
            break;
        }
    }
    CHECK(1);

    /* crude uniformity check: 6 buckets, 60000 draws */
    for (i = 0; i < 6; i++) {
        buckets[i] = 0;
    }
    rng_seed(&a, 2026u);
    for (i = 0; i < 60000; i++) {
        buckets[rng_range(&a, 6u)]++;
    }
    for (i = 0; i < 6; i++) {
        CHECK(buckets[i] > 9500 && buckets[i] < 10500);
    }
}

/* ---------------------------------------------------- Part 1: map ---- */
static void test_map(void)
{
    struct Game g;
    char err[MAP_ERR_LEN];
    static const char *ragged[]   = { "#####", "#P..#", "####" };
    static const char *no_pac[]   = { "#####", "#...#", "#####" };
    static const char *no_ghost[] = { "#####", "#P..#", "#####" };
    static const char *bad_char[] = { "#####", "#P.X#", "#####" };
    static const char *too_many[] = { "#######", "#PGGGG#", "#######" };
    static const char *sealed[]   = { "########", "#PG#...#", "########" };
    static const char *no_pellets[] = { "####", "#PG#", "####" };

    SUITE("Part 1 / map: embedded maps");

    CHECK_INT(map_embedded_count(), 5);
    CHECK(map_embedded(0) != NULL);
    CHECK(map_embedded(4) != NULL);
    CHECK(map_embedded(5) == NULL);
    CHECK(map_embedded(-1) == NULL);

    CHECK(new_embedded_game(&g, 1, 4, 2, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(g.rows, 11L);
    CHECK_INT(g.cols, 24L);
    CHECK_INT(g.ghostCount, 4L);
    CHECK_INT(g.pelletsTotal, map_count_pellets(&g));
    CHECK_INT(g.pelletsRemaining, g.pelletsTotal);
    CHECK_INT(map_reachable_pellets(&g), g.pelletsTotal);
    CHECK_INT(map_check(&g, err, (int)sizeof err), 0);
    CHECK_INT(g.pac.pos.r, g.pac.start.r);
    CHECK(!map_is_wall(&g, g.pac.start.r, g.pac.start.c));

    CHECK(new_embedded_game(&g, 2, 4, 2, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(g.rows, 24L);
    CHECK_INT(g.cols, 11L);
    CHECK_INT(map_check(&g, err, (int)sizeof err), 0);
    CHECK_INT(map_reachable_pellets(&g), g.pelletsTotal);
    CHECK(g.pelletsTotal > 0);

    /* every embedded level must be playable: all pellets reachable, 4 ghosts */
    {
        int level;
        for (level = 1; level <= map_embedded_count(); level++) {
            struct Game mg;
            char msg[MAP_ERR_LEN];
            CHECK(new_embedded_game(&mg, level, MAX_GHOSTS, 1, FEAT_ALL, AI_RANDOM) == 0);
            CHECK_INT(mg.ghostCount, 4L);
            CHECK_INT(map_check(&mg, msg, (int)sizeof msg), 0);
            CHECK_INT(map_reachable_pellets(&mg), mg.pelletsTotal);
            CHECK(mg.pelletsTotal > 40);
            CHECK(mg.rows <= MAP_MAX_ROWS && mg.cols <= MAP_MAX_COLS);
        }
    }

    CHECK(new_embedded_game(&g, 3, 4, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(g.rows, 13L);
    CHECK_INT(g.cols, 24L);
    CHECK(new_embedded_game(&g, 4, 4, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(g.rows, 13L);
    CHECK_INT(g.cols, 28L);
    CHECK(new_embedded_game(&g, 5, 4, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(g.rows, 13L);
    CHECK_INT(g.cols, 24L);

    SUITE("Part 1 / map: rejects broken maps");
    CHECK(load_inline(&g, ragged, 3, 1, 1, FEAT_ALL, AI_RANDOM) != 0);
    CHECK(load_inline(&g, no_pac, 3, 1, 1, FEAT_ALL, AI_RANDOM) != 0);
    CHECK(load_inline(&g, no_ghost, 3, 1, 1, FEAT_ALL, AI_RANDOM) != 0);
    CHECK(load_inline(&g, bad_char, 3, 1, 1, FEAT_ALL, AI_RANDOM) != 0);
    CHECK(load_inline(&g, too_many, 3, 1, 1, FEAT_ALL, AI_RANDOM) != 0);
    CHECK(load_inline(&g, no_pellets, 3, 1, 1, FEAT_ALL, AI_RANDOM) != 0);

    /* a sealed pocket full of pellets is accepted by the loader but must be
     * rejected by map_check because those pellets can never be eaten */
    CHECK(load_inline(&g, sealed, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK(map_check(&g, err, (int)sizeof err) != 0);

    SUITE("Part 1 / map: queries and file loading");
    CHECK(load_inline(&g, sealed, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_STR(g.mapName, "inline-test-map");
    CHECK(map_is_wall(&g, 0, 0));
    CHECK(!map_is_wall(&g, 1, 1));
    CHECK(map_is_wall(&g, -1, 0));          /* outside the board */
    CHECK(map_is_wall(&g, g.rows, 0));
    CHECK_INT(map_get(&g, 1, 1), (long)CELL_EMPTY);
    map_set(&g, 1, 1, CELL_PELLET);
    CHECK_INT(map_get(&g, 1, 1), (long)CELL_PELLET);
    CHECK_INT(map_get(&g, -5, -5), 0L);

    CHECK(file_map_is_valid("tests/cases/map_win.txt"));
    CHECK(file_map_is_valid("tests/cases/map_die.txt"));
    CHECK(file_map_is_valid("tests/cases/map_power.txt"));
    CHECK(!file_map_is_valid("tests/cases/does_not_exist.txt"));

    CHECK(load_file_map(&g, "tests/cases/map_win.txt", 4, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(g.pelletsTotal, 6L);
    CHECK_INT(g.ghostCount, 1L);            /* only one 'G' in the file  */
    CHECK_INT(g.ghostRequested, 4L);
}

/* ------------------------------------------------- Part 2: entity ---- */
static void test_entity(void)
{
    struct Game g;
    struct Entity e;
    static const char *maze[] = { "#######", "#P...G#", "#######" };

    SUITE("Part 2 / entity: movement and walls");

    CHECK(load_inline(&g, maze, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);

    CHECK(dir_is_unit(-1, 0));
    CHECK(dir_is_unit(1, 0));
    CHECK(dir_is_unit(0, -1));
    CHECK(dir_is_unit(0, 1));
    CHECK(!dir_is_unit(0, 0));
    CHECK(!dir_is_unit(1, 1));
    CHECK(!dir_is_unit(2, 0));

    e = g.pac;
    entity_face(&e, 5, 5);                  /* invalid direction is ignored */
    CHECK_INT(e.dr, 0L);
    CHECK_INT(e.dc, 0L);

    entity_face(&e, 0, 1);
    CHECK_INT(entity_step(&g, &e), 1L);
    CHECK_INT(e.pos.c, 2L);
    CHECK_INT(entity_step(&g, &e), 1L);
    CHECK_INT(e.pos.c, 3L);
    CHECK_INT(entity_step(&g, &e), 1L);
    CHECK_INT(e.pos.c, 4L);
    CHECK_INT(entity_step(&g, &e), 1L);
    CHECK_INT(e.pos.c, 5L);
    CHECK_INT(entity_step(&g, &e), 0L);     /* wall at column 6 */
    CHECK_INT(e.pos.c, 5L);                 /* ... and it did not move */

    entity_face(&e, -1, 0);
    CHECK_INT(entity_step(&g, &e), 0L);     /* wall above */
    CHECK_INT(e.pos.r, 1L);

    CHECK(!cell_is_open(&g, 0, 0));
    CHECK(cell_is_open(&g, 1, 1));
    CHECK(!cell_is_open(&g, -1, 1));

    CHECK(pos_equal(g.pac.pos, g.pac.start));
    {
        struct Pos other;
        other.r = 99;
        other.c = 99;
        CHECK(!pos_equal(g.pac.pos, other));
    }
}

/* -------------------------------------------------- Part 3: ghosts ---- */
static void test_ghosts(void)
{
    struct Game g;
    static const char *sealed_ghost[] = { "######", "#P.#G#", "######" };
    struct Rng driver;
    int i;
    int k;
    int ok;

    SUITE("Part 3 / ghost: never walks into a wall");

    CHECK(new_embedded_game(&g, 1, 4, 2, FEAT_ALL, AI_RANDOM) == 0);
    g.verbose = 0;
    g.aiMode = AI_RANDOM;
    rng_seed(&driver, 4242u);

    ok = 1;
    for (i = 0; i < 10000 && ok; i++) {
        int idx = i % g.ghostCount;
        ghost_update(&g, idx);
        for (k = 0; k < g.ghostCount; k++) {
            if (g.ghosts[k].state == GHOST_EATEN) {
                continue;
            }
            if (map_is_wall(&g, g.ghosts[k].pos.r, g.ghosts[k].pos.c)) {
                ok = 0;
            }
        }
    }
    CHECK(ok);

    SUITE("Part 3 / ghost: one cell per tick, no needless reversal");

    CHECK(new_embedded_game(&g, 1, 4, 2, FEAT_ALL, AI_RANDOM) == 0);
    g.verbose = 0;
    g.aiMode = AI_RANDOM;
    g.followLimit = 0;              /* test the plain no-reversal rule only */
    rng_seed(&driver, 777u);

    ok = 1;
    for (i = 0; i < 5000 && ok; i++) {
        struct Entity *gh = &g.ghosts[i % g.ghostCount];
        int before_r = gh->pos.r;
        int before_c = gh->pos.c;
        int before_dr = gh->dr;
        int before_dc = gh->dc;
        int legal_before = ghost_valid_dir_count(&g, gh);

        ghost_update(&g, i % g.ghostCount);

        if (gh->state == GHOST_EATEN) {
            continue;
        }
        if (abs(gh->pos.r - before_r) > 1 || abs(gh->pos.c - before_c) > 1) {
            ok = 0;                                  /* more than one cell */
        }
        if ((gh->dr == -before_dr && gh->dc == -before_dc) &&
            (before_dr != 0 || before_dc != 0) && legal_before > 1) {
            ok = 0;                                  /* needless reversal */
        }
        if (gh->dr != 0 && gh->dc != 0 && !(gh->dr == 0 && gh->dc == 0)) {
            if (gh->dr != 0 && gh->dc != 0) {
                ok = 0;                              /* diagonal move */
            }
        }
    }
    CHECK(ok);

    SUITE("Part 3 / ghost: reproducible for a fixed seed");

    {
        struct Game g1, g2;
        int same = 1;

        CHECK(new_embedded_game(&g1, 1, 4, 2, FEAT_ALL, AI_RANDOM) == 0);
        g1.verbose = 0;
        g1.rng.state = 1234u;
        CHECK(new_embedded_game(&g2, 1, 4, 2, FEAT_ALL, AI_RANDOM) == 0);
        g2.verbose = 0;
        g2.rng.state = 1234u;

        for (i = 0; i < 500; i++) {
            for (k = 0; k < g1.ghostCount; k++) {
                ghost_update(&g1, k);
                ghost_update(&g2, k);
                if (g1.ghosts[k].pos.r != g2.ghosts[k].pos.r ||
                    g1.ghosts[k].pos.c != g2.ghosts[k].pos.c) {
                    same = 0;
                }
            }
        }
        CHECK(same);
    }

    SUITE("Part 3 / ghost: sealed ghost stays where it is");

    CHECK(load_inline(&g, sealed_ghost, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(ghost_valid_dir_count(&g, &g.ghosts[0]), 0L);
    {
        int r0 = g.ghosts[0].pos.r;
        int c0 = g.ghosts[0].pos.c;
        for (i = 0; i < 50; i++) {
            ghost_update(&g, 0);
        }
        CHECK_INT(g.ghosts[0].pos.r, r0);
        CHECK_INT(g.ghosts[0].pos.c, c0);
    }
    SUITE("Part 3 / ghost: anti-follow fires at every following distance 1..5");

    {
        static const char *corridor[] = { "##############", "#PG..........#", "##############" };
        int distance;

        for (distance = 1; distance <= 5; distance++) {
            int t;

            CHECK(load_inline(&g, corridor, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
            g.followLimit = 3;                   /* the documented default */
            g.ghosts[0].pos.r = 1;
            g.ghosts[0].pos.c = 7;
            g.ghosts[0].dr = 0;
            g.ghosts[0].dc = 1;                  /* walking east, Pac-Man behind */
            g.pac.pos.r = 1;
            g.pac.pos.c = 7 - distance;

            for (t = 0; t < 2; t++) {
                /* keep the gap constant: Pac-Man will be `distance` behind
                 * the ghost after the ghost has moved one cell east */
                g.pac.pos.c = g.ghosts[0].pos.c + 1 - distance;
                ghost_update(&g, 0);
            }
            CHECK_INT(g.antiFollowFlips, 0);     /* two ticks are not enough */

            g.pac.pos.c = g.ghosts[0].pos.c + 1 - distance;
            ghost_update(&g, 0);                 /* third tick: it turns round */
            CHECK_INT(g.antiFollowFlips, 1);
            CHECK_INT(g.ghosts[0].followTicks, 0);

            {
                int before = g.ghosts[0].pos.c;
                g.pac.pos.c = g.ghosts[0].pos.c - 1;
                ghost_update(&g, 0);             /* next tick: the turn is applied */
                CHECK(g.ghosts[0].pos.c < before);
            }
        }
    }

    SUITE("Part 3 / ghost: anti-follow catches a stuttering follower");

    {
        static const char *corridor[] = { "##############", "#PG..........#", "##############" };
        int t;

        /* "move, wait, move": the gap grows to 3 and shrinks to 2 again, which
         * is exactly the trick that defeated the first version of the rule */
        CHECK(load_inline(&g, corridor, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
        g.followLimit = 3;
        g.ghosts[0].pos.r = 1;
        g.ghosts[0].pos.c = 7;
        g.ghosts[0].dr = 0;
        g.ghosts[0].dc = 1;
        g.pac.pos.r = 1;
        g.pac.pos.c = 5;

        for (t = 0; t < 3; t++) {
            if (t % 2 == 0) {
                g.pac.pos.c = g.ghosts[0].pos.c - 1;   /* step forward */
            }                                          /* else: wait one tick */
            ghost_update(&g, 0);
        }
        CHECK_INT(g.antiFollowFlips, 1);
    }

    SUITE("Part 3 / ghost: anti-follow stays silent when it should");

    {
        static const char *corridor[] = { "##############", "#PG..........#", "##############" };
        int t;

        /* 1. Pac-Man chasing a scared ghost must stay possible */
        CHECK(load_inline(&g, corridor, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
        g.followLimit = 3;
        g.ghosts[0].state = GHOST_SCARED;
        g.ghosts[0].pos.r = 1;
        g.ghosts[0].pos.c = 4;
        g.ghosts[0].dr = 0;
        g.ghosts[0].dc = 1;
        for (t = 0; t < 20; t++) {
            g.pac.pos.r = 1;
            g.pac.pos.c = g.ghosts[0].pos.c - 1;
            if (g.pac.pos.c < 1) {
                break;
            }
            ghost_update(&g, 0);
        }
        CHECK_INT(g.antiFollowFlips, 0);

        /* 2. Pac-Man in the parallel corridor: a wall between them */
        {
            static const char *parallel[] = {
                "##############",
                "#PG..........#",
                "##############",
                "#............#",
                "##############"
            };

            CHECK(load_inline(&g, parallel, 5, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
            g.followLimit = 3;
            g.ghosts[0].pos.r = 1;
            g.ghosts[0].pos.c = 2;
            g.ghosts[0].dr = 0;
            g.ghosts[0].dc = 1;
            for (t = 0; t < 20; t++) {
                g.pac.pos.r = 3;
                g.pac.pos.c = g.ghosts[0].pos.c;
                ghost_update(&g, 0);
            }
            CHECK_INT(g.antiFollowFlips, 0);
        }

        /* 3. the ghost is closing in, so it is not being followed */
        CHECK(load_inline(&g, corridor, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
        g.followLimit = 3;
        g.ghosts[0].pos.r = 1;
        g.ghosts[0].pos.c = 7;
        g.ghosts[0].dr = 0;
        g.ghosts[0].dc = -1;                 /* walking towards Pac-Man */
        for (t = 0; t < 4; t++) {
            g.pac.pos.r = 1;
            g.pac.pos.c = 2;
            ghost_update(&g, 0);
        }
        CHECK_INT(g.antiFollowFlips, 0);

        /* 4. the rule can be switched off (--no-antifollow) */
        CHECK(load_inline(&g, corridor, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
        g.followLimit = 0;
        g.ghosts[0].pos.r = 1;
        g.ghosts[0].pos.c = 2;
        g.ghosts[0].dr = 0;
        g.ghosts[0].dc = 1;
        for (t = 0; t < 20; t++) {
            g.pac.pos.r = 1;
            g.pac.pos.c = g.ghosts[0].pos.c - 1;
            if (g.pac.pos.c < 1) {
                break;
            }
            ghost_update(&g, 0);
        }
        CHECK_INT(g.antiFollowFlips, 0);
    }
}

/* --------------------------------------------------- Part 4: game ---- */
static void test_game(void)
{
    struct Game g;
    enum GameStatus st;
    static const char *one_pellet[] = { "#######", "#P..#G#", "#######" };
    static const char *power_map[]  = { "#######", "#OPG..#", "#######" };

    SUITE("Part 4 / game: initial state");

    CHECK(load_inline(&g, one_pellet, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    CHECK_INT(g.lives, START_LIVES);
    CHECK_INT(g.score, 0L);
    CHECK_INT(g.tick, 0L);
    CHECK_INT(g.pelletsRemaining, 2L);
    CHECK_INT(g.pelletsEatenTotal, 0L);

    SUITE("Part 4 / game: eating pellets");

    st = game_tick(&g, 0, 1);                 /* move right onto a pellet */
    CHECK_INT(st, GAME_RUNNING);
    CHECK_INT(g.score, PELLET_POINTS);
    CHECK_INT(g.pelletsRemaining, 1L);
    CHECK_INT(g.pelletsEatenTotal, 1L);
    CHECK_INT(g.tick, 1L);
    CHECK_INT(map_get(&g, 1, 2), (long)CELL_EMPTY);

    SUITE("Part 4 / game: wall bump still advances the tick");

    CHECK_INT(game_tick(&g, -1, 0), GAME_RUNNING);   /* wall above */
    CHECK_INT(g.pac.pos.r, 1L);
    CHECK_INT(g.pac.pos.c, 2L);
    CHECK_INT(g.tick, 2L);

    SUITE("Part 4 / game: win when the last pellet is eaten");

    CHECK_INT(game_tick(&g, 0, 1), GAME_WIN);
    CHECK_INT(g.pelletsRemaining, 0L);
    CHECK_INT(g.score, 2 * PELLET_POINTS);

    SUITE("Part 4 / game: death, respawn and game over");

    CHECK(load_inline(&g, one_pellet, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    /* put a normal ghost on the cell Pac-Man is about to enter */
    g.ghosts[0].state = GHOST_NORMAL;
    g.ghosts[0].pos.r = 1;
    g.ghosts[0].pos.c = 2;
    st = game_tick(&g, 0, 1);
    CHECK_INT(st, GAME_RUNNING);
    CHECK_INT(g.lives, START_LIVES - 1);
    CHECK_INT(g.pac.pos.r, g.pac.start.r);           /* respawned */
    CHECK_INT(g.pac.pos.c, g.pac.start.c);
    CHECK_INT(g.ghosts[0].pos.r, g.ghosts[0].start.r);
    CHECK_INT(g.ghosts[0].pos.c, g.ghosts[0].start.c);

    /* three deaths in a row must end the game */
    g.ghosts[0].pos.r = 1; g.ghosts[0].pos.c = 2;
    game_tick(&g, 0, 1);
    g.ghosts[0].pos.r = 1; g.ghosts[0].pos.c = 2;
    st = game_tick(&g, 0, 1);
    CHECK_INT(st, GAME_LOSE);
    CHECK_INT(g.lives, 0L);

    SUITE("Part 4 / game: pellets stay eaten after a death");

    CHECK(load_inline(&g, one_pellet, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    game_tick(&g, 0, 1);                              /* eat one pellet */
    CHECK_INT(g.pelletsRemaining, 1L);
    /* the ghost now stands in front of Pac-Man and has no other move, so a
     * wall bump makes it walk onto Pac-Man and cost a life */
    g.ghosts[0].pos.r = 1;
    g.ghosts[0].pos.c = 3;
    game_tick(&g, -1, 0);                             /* wall above */
    CHECK_INT(g.lives, START_LIVES - 1);
    CHECK_INT(g.pelletsRemaining, 1L);                /* not restored */

    SUITE("Part 4 / game: power pellet and scared ghosts");

    CHECK(load_inline(&g, power_map, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    game_tick(&g, 0, -1);                             /* step onto the 'O' */
    CHECK_INT(g.scaredTicks, SCARED_TICKS - 1);       /* decremented this tick */
    CHECK_INT(g.ghosts[0].state, GHOST_SCARED);
    CHECK_INT(g.score, POWER_PELLET_POINTS);
    CHECK_INT(g.pelletsRemaining, 2L);

    SUITE("Part 4 / game: eating a scared ghost scores and removes it");

    CHECK(load_inline(&g, power_map, 3, 1, 1, FEAT_ALL, AI_RANDOM) == 0);
    g.ghosts[0].state = GHOST_SCARED;
    g.ghosts[0].respawnIn = 0;
    g.ghosts[0].pos.r = 1;
    g.ghosts[0].pos.c = 3;                            /* the cell Pac-Man enters */
    g.scaredTicks = SCARED_TICKS;
    game_tick(&g, 0, 1);
    CHECK_INT(g.score, GHOST_POINTS_BASE);
    CHECK_INT(g.ghosts[0].state, GHOST_EATEN);
    CHECK_INT(g.lives, START_LIVES);                  /* no life lost */
    CHECK_INT(g.ghostChain, 1L);

    {
        int i;
        for (i = 0; i < GHOST_RESPAWN_TICKS; i++) {
            ghost_update(&g, 0);
        }
        CHECK_INT(g.ghosts[0].state, GHOST_NORMAL);
        CHECK_INT(g.ghosts[0].pos.r, g.ghosts[0].start.r);
    }

    SUITE("Part 4 / game: core mode treats 'O' as a normal pellet");

    CHECK(load_inline(&g, power_map, 3, 1, 1, 0, AI_RANDOM) == 0);
    game_tick(&g, 0, -1);
    CHECK_INT(g.score, PELLET_POINTS);
    CHECK_INT(g.scaredTicks, 0L);
    CHECK_INT(g.ghosts[0].state, GHOST_NORMAL);

    SUITE("Part 4 / game: level transition");

    CHECK(load_inline(&g, one_pellet, 3, 1, 2, FEAT_LEVELS, AI_RANDOM) == 0);
    CHECK_INT(g.level, 1L);
    game_tick(&g, 0, 1);
    st = game_tick(&g, 0, 1);
    CHECK_INT(st, GAME_RUNNING);
    CHECK_INT(g.level, 2L);
    CHECK_INT(g.rows, 24L);                            /* embedded level 2 */
    CHECK_INT(g.cols, 11L);
    CHECK(g.pelletsRemaining > 0);
    CHECK_INT(g.pelletsEatenTotal, 2L);                /* carried over */

    SUITE("Part 4 / game: difficulty table of the start menu");

    {
        int lv = 0;
        int gh = 0;

        game_difficulty(1, &lv, &gh);  CHECK_INT(lv, 1L); CHECK_INT(gh, 2L);
        game_difficulty(2, &lv, &gh);  CHECK_INT(lv, 2L); CHECK_INT(gh, 2L);
        game_difficulty(3, &lv, &gh);  CHECK_INT(lv, 3L); CHECK_INT(gh, 3L);
        game_difficulty(4, &lv, &gh);  CHECK_INT(lv, 4L); CHECK_INT(gh, 3L);
        game_difficulty(5, &lv, &gh);  CHECK_INT(lv, 5L); CHECK_INT(gh, 4L);
        game_difficulty(0, &lv, &gh);  CHECK_INT(lv, 1L); CHECK_INT(gh, 2L);
        game_difficulty(99, &lv, &gh); CHECK_INT(lv, 5L); CHECK_INT(gh, 4L);
        CHECK_INT(DIFFICULTY_MAX, 5L);
        CHECK_INT(FOLLOW_RANGE, 5L);      /* the values documented above */
        CHECK_INT(FOLLOW_TICKS, 3L);
        CHECK_INT(FOLLOW_COOLDOWN, 2L);
    }

    SUITE("Part 4 / game: fruit extension");

    CHECK(new_embedded_game(&g, 1, 1, 2, FEAT_ALL, AI_RANDOM) == 0);
    g.verbose = 0;
    g.flags = FEAT_FRUIT | FEAT_LEVELS;
    g.lives = 99;                       /* survive long enough to see fruit */
    {
        int spawn_seen = 0;
        int t;
        for (t = 0; t < 200; t++) {
            game_tick(&g, 0, 1);
            if (g.fruitActive) {
                spawn_seen = 1;
                CHECK(!map_is_wall(&g, g.fruitPos.r, g.fruitPos.c));
                CHECK(g.fruitPos.r >= 0 && g.fruitPos.r < g.rows);
                CHECK(g.fruitPos.c >= 0 && g.fruitPos.c < g.cols);
                CHECK(!(g.fruitPos.r == g.pac.pos.r && g.fruitPos.c == g.pac.pos.c));
                break;
            }
        }
        CHECK(spawn_seen);
    }

    /* deterministic fruit eating: put the fruit on an open cell next to
     * Pac-Man and walk into it */
    {
        int before;
        int dc = map_is_wall(&g, g.pac.pos.r, g.pac.pos.c + 1) ? -1 : 1;

        CHECK(!map_is_wall(&g, g.pac.pos.r, g.pac.pos.c + dc));

        g.fruitActive = 1;
        g.fruitTicksLeft = FRUIT_LIFETIME;
        g.fruitPos.r = g.pac.pos.r;
        g.fruitPos.c = g.pac.pos.c + dc;
        before = g.score;

        game_tick(&g, 0, dc);
        CHECK_INT(g.fruitActive, 0L);                     /* eaten */
        CHECK(g.score >= before + FRUIT_POINTS);
    }
}

/* -------------------------------------------------- Part 2: input ---- */
static void test_input(void)
{
    FILE *fp;
    struct Input in;

    SUITE("Part 2 / input: key mapping, invalid keys and EOF");

    /* a scratch file is used instead of tmpfile(): tmpfile() is not
     * available in every lab environment, a real file always works */
    fp = fopen("tests/logs/_tmp_input.txt", "w+");
    CHECK(fp != NULL);
    if (fp == NULL) {
        return;
    }

    fputs("w\na\ns\nd\n", fp);
    fputs("W\n", fp);            /* upper case accepted   */
    fputs("\n", fp);             /* empty line ignored    */
    fputs("   \n", fp);          /* blanks ignored        */
    fputs("x\nhello\n", fp);     /* both invalid first characters */
    fputs("q\n", fp);
    rewind(fp);

    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_MOVE);
    CHECK_INT(in.dr, -1L); CHECK_INT(in.dc, 0L);

    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_MOVE);
    CHECK_INT(in.dr, 0L); CHECK_INT(in.dc, -1L);

    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_MOVE);
    CHECK_INT(in.dr, 1L); CHECK_INT(in.dc, 0L);

    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_MOVE);
    CHECK_INT(in.dr, 0L); CHECK_INT(in.dc, 1L);

    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_MOVE);
    CHECK_INT(in.dr, -1L);

    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_INVALID);       /* empty line */
    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_INVALID);       /* blank line */
    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_INVALID);       /* 'x' */
    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_INVALID);       /* 'h' of "hello" */
    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_QUIT);

    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_EOF);           /* must not crash  */
    in = input_read(fp);
    CHECK_INT(in.kind, INPUT_EOF);

    fclose(fp);
    remove("tests/logs/_tmp_input.txt");
}

/* --------------------------------------------------- Part 1: screen ---- */
static void test_screen(void)
{
    struct Screen s;
    FILE *fp;
    char buffer[8192];
    size_t got;

    SUITE("Part 1 / screen: frame buffer and colour output");

    screen_init(&s, 5, 10);
    CHECK_INT(s.rows, 5L);
    CHECK_INT(s.cols, 10L);
    CHECK_INT(s.ch[0][0], ' ');
    CHECK_INT(s.color[0][0], (long)C_NONE);

    screen_put(&s, 0, 0, "hi", C_HUD);
    CHECK_INT(s.ch[0][0], 'h');
    CHECK_INT(s.ch[0][1], 'i');
    CHECK_INT(s.color[0][1], (long)C_HUD);

    /* clips at the right edge and ignores rows outside the frame */
    screen_put(&s, 0, 8, "abcd", C_HUD);
    CHECK_INT(s.ch[0][8], 'a');
    CHECK_INT(s.ch[0][9], 'b');
    screen_put(&s, 99, 0, "x", C_HUD);
    screen_put(&s, -1, 0, "x", C_HUD);
    CHECK(1);

    /* a box puts '+' in the corners and '-' / '|' on the edges */
    screen_clear(&s);
    screen_box(&s, 0, 0, 4, 9, C_BORDER);
    CHECK_INT(s.ch[0][0], '+');
    CHECK_INT(s.ch[0][9], '+');
    CHECK_INT(s.ch[4][0], '+');
    CHECK_INT(s.ch[0][4], '-');
    CHECK_INT(s.ch[2][0], '|');
    CHECK_INT(s.ch[2][9], '|');
    CHECK_INT(s.color[2][0], (long)C_BORDER);

    /* flush with colour: escape sequences present */
    fp = fopen("tests/logs/_tmp_screen.txt", "w+");
    CHECK(fp != NULL);
    if (fp != NULL) {
        screen_put(&s, 2, 2, "X", C_PAC);
        screen_flush(&s, fp, 1);
        fflush(fp);
        rewind(fp);
        got = fread(buffer, 1, sizeof buffer - 1, fp);
        buffer[got] = '\0';
        fclose(fp);
        CHECK(strstr(buffer, "\x1b[") != NULL);      /* colours emitted  */
        CHECK(strstr(buffer, "\x1b[H") != NULL);     /* cursor homed     */
        CHECK(strchr(buffer, 'X') != NULL);

        /* flush without colour: pure text, no escapes */
        fp = fopen("tests/logs/_tmp_screen.txt", "w+");
        if (fp != NULL) {
            screen_flush(&s, fp, 0);
            fflush(fp);
            rewind(fp);
            got = fread(buffer, 1, sizeof buffer - 1, fp);
            buffer[got] = '\0';
            fclose(fp);
            CHECK(strstr(buffer, "\x1b[") == NULL);
            CHECK(strchr(buffer, 'X') != NULL);
            CHECK(strchr(buffer, '+') != NULL);
        }
        remove("tests/logs/_tmp_screen.txt");
    }
}

/* --------------------------------------------------- rendering ------- */
static void test_render(void)
{
    struct Game g;
    char buffer[4096];
    FILE *fp;

    SUITE("Part 1 / render: board is drawn with the documented symbols");

    CHECK(new_embedded_game(&g, 1, 4, 2, FEAT_ALL, AI_RANDOM) == 0);
    g.verbose = 0;

    fp = fopen("tests/logs/_tmp_render.txt", "w+");
    CHECK(fp != NULL);
    if (fp == NULL) {
        return;
    }
    render_banner(&g, fp);
    render_all(&g, fp);
    fflush(fp);
    rewind(fp);

    {
        size_t got = fread(buffer, 1, sizeof buffer - 1, fp);
        buffer[got] = '\0';
    }
    fclose(fp);
    remove("tests/logs/_tmp_render.txt");

    CHECK(strstr(buffer, "PAC-MAN") != NULL);
    CHECK(strstr(buffer, "Level 1/2") != NULL);
    CHECK(strstr(buffer, "Pellets") != NULL);
    CHECK(strstr(buffer, "Tick 0") != NULL);
    /* the first board row is the top wall of the embedded map */
    CHECK(strstr(buffer, "########################") != NULL);
    CHECK(strstr(buffer, "P") != NULL);       /* Pac-Man visible */
    CHECK(strstr(buffer, "G") != NULL);       /* ghost visible   */

    SUITE("Part 1 / render: a scared ghost is drawn as '!'");

    {
        char board[4096];
        size_t got;
        int t;

        g.ghosts[0].state = GHOST_SCARED;
        g.ghosts[0].pos.r = g.pac.pos.r - 1;
        g.ghosts[0].pos.c = g.pac.pos.c;

        fp = fopen("tests/logs/_tmp_render.txt", "w+");
        CHECK(fp != NULL);
        if (fp != NULL) {
            render_board(&g, fp);
            fflush(fp);
            rewind(fp);
            got = fread(board, 1, sizeof board - 1, fp);
            board[got] = '\0';
            fclose(fp);
            remove("tests/logs/_tmp_render.txt");

            CHECK(strstr(board, "!") != NULL);      /* scared symbol */
            CHECK(strchr(board, 'g') == NULL);      /* no lower case g */

            /* back to normal: the same cell must show 'G' again */
            g.ghosts[0].state = GHOST_NORMAL;
            fp = fopen("tests/logs/_tmp_render.txt", "w+");
            if (fp != NULL) {
                render_board(&g, fp);
                fflush(fp);
                rewind(fp);
                got = fread(board, 1, sizeof board - 1, fp);
                board[got] = '\0';
                fclose(fp);
                remove("tests/logs/_tmp_render.txt");
                CHECK(strchr(board, 'G') != NULL);
                CHECK(strchr(board, '!') == NULL);
            }
        }
        (void)t;
    }

    SUITE("Part 1 / render: framed display and title screen");

    {
        char frame[16384];
        size_t got;

        g.ghosts[0].state = GHOST_NORMAL;
        fp = fopen("tests/logs/_tmp_frame.txt", "w+");
        CHECK(fp != NULL);
        if (fp != NULL) {
            render_frame(&g, fp, 1);
            fflush(fp);
            rewind(fp);
            got = fread(frame, 1, sizeof frame - 1, fp);
            frame[got] = '\0';
            fclose(fp);
            remove("tests/logs/_tmp_frame.txt");

            CHECK(strstr(frame, "\x1b[") != NULL);      /* colours      */
            CHECK(strstr(frame, "PAC-MAN") != NULL);    /* title in box */
            CHECK(strstr(frame, "LEVEL 1/2") != NULL);
            CHECK(strstr(frame, "SCORE") != NULL);
            CHECK(strstr(frame, "LIVES") != NULL);
            CHECK(strstr(frame, "PELLETS") != NULL);
            CHECK(strstr(frame, "TICK") != NULL);
        }

        /* the same picture without colour must be plain ASCII */
        fp = fopen("tests/logs/_tmp_frame.txt", "w+");
        if (fp != NULL) {
            render_frame(&g, fp, 0);
            fflush(fp);
            rewind(fp);
            got = fread(frame, 1, sizeof frame - 1, fp);
            frame[got] = '\0';
            fclose(fp);
            remove("tests/logs/_tmp_frame.txt");
            CHECK(strstr(frame, "\x1b[") == NULL);
            CHECK(strstr(frame, "+- PAC-MAN ") != NULL);
            CHECK(strstr(frame, "SCORE") != NULL);
        }

        /* the title screen carries the art and the menu hint */
        fp = fopen("tests/logs/_tmp_frame.txt", "w+");
        if (fp != NULL) {
            render_title(fp, 0);
            fflush(fp);
            rewind(fp);
            got = fread(frame, 1, sizeof frame - 1, fp);
            frame[got] = '\0';
            fclose(fp);
            remove("tests/logs/_tmp_frame.txt");
            CHECK(strstr(frame, "____") != NULL);           /* the art      */
            CHECK(strstr(frame, "[1]-[5]") != NULL);        /* menu hint    */
            CHECK(strstr(frame, "Group 6") != NULL);
        }
    }
}

/* ------------------------------------------------------------- main -- */
int main(void)
{
    printf("MFE113TC Pac-Man - unit test suite\n");
    printf("=================================\n");

    test_rng();
    test_map();
    test_entity();
    test_ghosts();
    test_game();
    test_input();
    test_screen();
    test_render();

    printf("=================================\n");
    printf("checks run: %d, failed: %d\n", checks_run, checks_failed);
    if (checks_failed == 0) {
        printf("UNIT TESTS: ALL PASSED\n");
        return 0;
    }
    printf("UNIT TESTS: FAILED\n");
    return 1;
}
