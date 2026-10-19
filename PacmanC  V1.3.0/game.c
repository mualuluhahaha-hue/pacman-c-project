/*
 * game.c - game state, tick order and rules.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing); the collision rules also belong to Part 2.
 *
 * Tick order (fixed design decision, see report section 1.7):
 *   1. Pac-Man moves; a wall keeps it in place but the tick still advances.
 *   2. Pac-Man eats whatever is on its new cell (pellet / power pellet / fruit).
 *   3. Collision check - Pac-Man may have walked into a ghost.
 *   4. Win check - the level (or the game) is won when no pellet is left, and
 *      this is checked BEFORE the ghosts move, so the last pellet can never be
 *      taken away by an unlucky ghost step.
 *   5. Timers: scared countdown, chase/scatter alternation, fruit lifetime.
 *   6. Every ghost moves exactly once.
 *   7. Collision check again - a ghost may have walked into Pac-Man.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "map.h"
#include "entity.h"
#include "ghost.h"

/* ------------------------------------------------------------- helpers */
static void eventf(const struct Game *g, const char *fmt, ...)
{
    va_list ap;

    if (!g->verbose) {
        return;
    }
    printf("  * ");
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

static int clamp_int(int value, int lo, int hi)
{
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

/* ---------------------------------------------------------- life cycle */
void game_init(struct Game *g, unsigned int seed, int ghosts, int levels,
               int flags, int aiMode)
{
    memset(g, 0, sizeof *g);

    g->rngSeed = seed;
    rng_seed(&g->rng, (uint32_t)seed);

    g->ghostRequested = clamp_int(ghosts, 1, MAX_GHOSTS);
    g->levels = clamp_int(levels, 1, MAX_LEVELS);
    g->flags = flags;
    g->aiMode = aiMode;

    g->lives = START_LIVES;
    g->score = 0;
    g->tick = 0;
    g->level = 0;
    g->verbose = 1;

    g->globalMode = MODE_SCATTER;
    g->modeTimer = SCATTER_TICKS;
    g->fruitSpawnIn = FRUIT_FIRST_TICK;
    g->mapName[0] = '\0';

    g->followLimit = FOLLOW_TICKS;
    g->followRange = FOLLOW_RANGE;
    g->antiFollowFlips = 0;
    g->difficulty = 0;
}

/*
 * Difficulty table of the start menu.
 *
 *   1 easy     1 level,  2 ghosts
 *   2 normal   2 levels, 2 ghosts
 *   3 hard     3 levels, 3 ghosts
 *   4 harder   4 levels, 3 ghosts
 *   5 expert   5 levels, 4 ghosts
 *
 * "Levels" is the number of mazes that have to be cleared (the five embedded
 * maps in their order), so a higher difficulty also means a longer game.
 */
void game_difficulty(int choice, int *levels, int *ghosts)
{
    static const struct {
        int levels;
        int ghosts;
    } TABLE[DIFFICULTY_MAX] = {
        { 1, 2 },   /* 1 easy   */
        { 2, 2 },   /* 2 normal */
        { 3, 3 },   /* 3 hard   */
        { 4, 3 },   /* 4 harder */
        { 5, 4 }    /* 5 expert */
    };
    int index;

    if (choice < DIFFICULTY_MIN) {
        choice = DIFFICULTY_MIN;
    }
    if (choice > DIFFICULTY_MAX) {
        choice = DIFFICULTY_MAX;
    }
    index = choice - 1;

    if (levels != NULL) {
        *levels = TABLE[index].levels;
    }
    if (ghosts != NULL) {
        *ghosts = TABLE[index].ghosts;
    }
}

int game_load_level(struct Game *g, int level)
{
    const struct MapDef *def;
    char err[MAP_ERR_LEN];

    if (level < 1 || level > map_embedded_count()) {
        printf("internal error: level %d does not exist\n", level);
        return -1;
    }

    def = map_embedded(level - 1);
    if (map_load_def(g, def, err, (int)sizeof err) != 0) {
        printf("map error: %s\n", err);
        return -1;
    }
    if (map_check(g, err, (int)sizeof err) != 0) {
        printf("map rejected: %s\n", err);
        return -1;
    }

    g->level = level;

    /* per-level extension state */
    g->scaredTicks = 0;
    g->ghostChain = 0;
    g->globalMode = MODE_SCATTER;
    g->modeTimer = SCATTER_TICKS;
    g->fruitActive = 0;
    g->fruitSpawnIn = FRUIT_FIRST_TICK;
    g->fruitTicksLeft = 0;

    /* the map may offer fewer ghost cells than the player asked for */
    if (g->ghostCount > g->ghostRequested) {
        g->ghostCount = g->ghostRequested;
    }

    return 0;
}

/* ----------------------------------------------------------- collision */
/* Returns 1 when Pac-Man and a normal ghost share a cell (Pac-Man dies).
 * Scared ghosts that share the cell are eaten instead. */
static int collision_check(struct Game *g)
{
    int i;
    int death = 0;

    for (i = 0; i < g->ghostCount; i++) {
        struct Entity *gh = &g->ghosts[i];

        if (gh->state == GHOST_EATEN) {
            continue;
        }
        if (!pos_equal(gh->pos, g->pac.pos)) {
            continue;
        }

        if (gh->state == GHOST_SCARED) {
            int points = GHOST_POINTS_BASE << g->ghostChain;

            if (g->ghostChain < GHOST_CHAIN_MAX) {
                g->ghostChain++;
            }
            g->score += points;
            gh->state = GHOST_EATEN;
            gh->respawnIn = GHOST_RESPAWN_TICKS;
            gh->followTicks = 0;
            gh->forceReverse = 0;
            eventf(g, "Ghost %d eaten! +%d points (returns in %d ticks)",
                   i + 1, points, GHOST_RESPAWN_TICKS);
        } else {
            death = 1;
        }
    }

    return death;
}

static void game_respawn(struct Game *g)
{
    int i;

    g->pac.pos = g->pac.start;
    g->pac.dr = 0;
    g->pac.dc = 0;

    ghost_reset_all(g);

    for (i = 0; i < g->ghostCount; i++) {
        g->ghosts[i].state = GHOST_NORMAL;
        g->ghosts[i].respawnIn = 0;
    }

    g->scaredTicks = 0;
    g->ghostChain = 0;
    g->fruitActive = 0;
    g->fruitSpawnIn = FRUIT_INTERVAL;

    eventf(g, "Pac-Man and the ghosts respawn at their start cells "
              "(pellets stay eaten)");
}

/* Handles one death.  Returns 1 when the game continues, 0 when it is over. */
static int lose_life(struct Game *g)
{
    g->lives--;
    eventf(g, "Pac-Man was caught! Lives left: %d", g->lives);

    if (g->lives <= 0) {
        return 0;
    }
    game_respawn(g);
    return 1;
}

/* ------------------------------------------------------------ consume */
static void game_consume(struct Game *g)
{
    struct Pos p = g->pac.pos;
    char ch = map_get(g, p.r, p.c);
    int i;

    if (ch == CELL_PELLET) {
        map_set(g, p.r, p.c, CELL_EMPTY);
        g->score += PELLET_POINTS;
        g->pelletsRemaining--;
        g->pelletsEatenTotal++;
        eventf(g, "Pellet eaten at (%d,%d), score %d, %d left",
               p.r, p.c, g->score, g->pelletsRemaining);
    } else if (ch == CELL_POWER) {
        map_set(g, p.r, p.c, CELL_EMPTY);
        g->pelletsRemaining--;
        g->pelletsEatenTotal++;

        if (g->flags & FEAT_POWER) {
            g->score += POWER_PELLET_POINTS;
            g->scaredTicks = SCARED_TICKS;
            g->ghostChain = 0;
            for (i = 0; i < g->ghostCount; i++) {
                if (g->ghosts[i].state == GHOST_NORMAL) {
                    g->ghosts[i].state = GHOST_SCARED;
                }
            }
            eventf(g, "Power pellet! ghosts are scared for %d ticks (+%d points)",
                   SCARED_TICKS, POWER_PELLET_POINTS);
        } else {
            /* core mode: 'O' is an ordinary pellet worth the normal points */
            g->score += PELLET_POINTS;
            eventf(g, "Power pellet treated as a normal pellet (core mode)");
        }
    }

    if (g->fruitActive && pos_equal(p, g->fruitPos)) {
        g->score += FRUIT_POINTS;
        g->fruitActive = 0;
        g->fruitSpawnIn = FRUIT_INTERVAL;
        eventf(g, "Fruit eaten! +%d points", FRUIT_POINTS);
    }
}

/* --------------------------------------------------------------- fruit */
static int cell_occupied(const struct Game *g, int r, int c)
{
    int i;

    if (g->pac.pos.r == r && g->pac.pos.c == c) {
        return 1;
    }
    for (i = 0; i < g->ghostCount; i++) {
        if (g->ghosts[i].state != GHOST_EATEN &&
            g->ghosts[i].pos.r == r && g->ghosts[i].pos.c == c) {
            return 1;
        }
    }
    return 0;
}

static int spawn_fruit(struct Game *g)
{
    int attempt;

    for (attempt = 0; attempt < 200; attempt++) {
        int r = (int)rng_range(&g->rng, (uint32_t)g->rows);
        int c = (int)rng_range(&g->rng, (uint32_t)g->cols);

        if (map_is_wall(g, r, c)) {
            continue;
        }
        if (cell_occupied(g, r, c)) {
            continue;
        }

        g->fruitPos.r = r;
        g->fruitPos.c = c;
        eventf(g, "Fruit appeared at (%d,%d) for %d ticks", r, c, FRUIT_LIFETIME);
        return 1;
    }
    return 0;
}

static void update_fruit(struct Game *g)
{
    if (g->fruitActive) {
        if (g->fruitTicksLeft > 0) {
            g->fruitTicksLeft--;
        }
        if (g->fruitTicksLeft == 0) {
            g->fruitActive = 0;
            g->fruitSpawnIn = FRUIT_INTERVAL;
            eventf(g, "Fruit expired");
        }
        return;
    }

    if (g->fruitSpawnIn > 0) {
        g->fruitSpawnIn--;
    }
    if (g->fruitSpawnIn == 0) {
        if (spawn_fruit(g)) {
            g->fruitActive = 1;
            g->fruitTicksLeft = FRUIT_LIFETIME;
        } else {
            g->fruitSpawnIn = 1;            /* board full: try again next tick */
        }
    }
}

/* ---------------------------------------------------------------- tick */
enum GameStatus game_tick(struct Game *g, int dr, int dc)
{
    int i;

    g->tick++;               /* this tick is being played now */

    /* 1 - Pac-Man moves (wall => stays in place) */
    entity_face(&g->pac, dr, dc);
    (void)entity_step(g, &g->pac);

    /* 2 - eat pellet / power pellet / fruit */
    game_consume(g);

    /* 3 - Pac-Man may have walked onto a ghost */
    if (collision_check(g)) {
        if (!lose_life(g)) {
            return GAME_LOSE;
        }
        return GAME_RUNNING;
    }

    /* 4 - win check (before the ghosts move) */
    if (g->pelletsRemaining == 0) {
        if ((g->flags & FEAT_LEVELS) && g->level < g->levels) {
            eventf(g, "Level %d cleared! Score so far: %d", g->level, g->score);
            if (game_load_level(g, g->level + 1) != 0) {
                return GAME_QUIT;
            }
            printf("\n=== LEVEL %d ===\n", g->level);
            return GAME_RUNNING;
        }
        return GAME_WIN;
    }

    /* 5 - timers */
    if (g->scaredTicks > 0) {
        g->scaredTicks--;
        if (g->scaredTicks == 0) {
            g->ghostChain = 0;
            for (i = 0; i < g->ghostCount; i++) {
                if (g->ghosts[i].state == GHOST_SCARED) {
                    g->ghosts[i].state = GHOST_NORMAL;
                }
            }
            eventf(g, "The ghosts are no longer scared");
        }
    }
    if ((g->flags & FEAT_MODES) && g->aiMode == AI_SMART) {
        ghost_update_global_mode(g);
    }
    if (g->flags & FEAT_FRUIT) {
        update_fruit(g);
    }

    /* 6 - every ghost moves once */
    for (i = 0; i < g->ghostCount; i++) {
        ghost_update(g, i);
    }

    /* 7 - a ghost may have walked onto Pac-Man */
    if (collision_check(g)) {
        if (!lose_life(g)) {
            return GAME_LOSE;
        }
        return GAME_RUNNING;
    }

    return GAME_RUNNING;
}

int game_fruit_active(const struct Game *g)
{
    return g->fruitActive;
}

/* ------------------------------------------------------------- summary */
static const char *status_name(enum GameStatus status)
{
    switch (status) {
    case GAME_WIN:  return "WIN";
    case GAME_LOSE: return "LOSE";
    case GAME_QUIT: return "QUIT";
    default:        return "RUNNING";
    }
}

void game_print_summary(const struct Game *g, enum GameStatus status, FILE *out)
{
    fprintf(out, "\n========== GAME OVER ==========\n");
    fprintf(out, "Result        : %s\n", status_name(status));
    fprintf(out, "Level reached : %d of %d\n", g->level, g->levels);
    fprintf(out, "Final score   : %d\n", g->score);
    fprintf(out, "Pellets eaten : %d (this level: %d of %d)\n",
            g->pelletsEatenTotal,
            g->pelletsTotal - g->pelletsRemaining, g->pelletsTotal);
    fprintf(out, "Ticks played  : %d\n", g->tick);
    fprintf(out, "Lives left    : %d\n", g->lives);
    fprintf(out, "Map           : %s\n", g->mapName);
    fprintf(out, "Difficulty    : %d\n", g->difficulty);
    fprintf(out, "Config        : seed=%u ghosts=%d ai=%s%s\n",
            g->rngSeed, g->ghostCount,
            (g->aiMode == AI_SMART) ? "smart" :
            (g->aiMode == AI_PURE) ? "pure" : "random",
            (g->flags == FEAT_ALL) ? "" : " core-mode");

    /* one machine readable line - the test runner greps this */
    fprintf(out, "RESULT=%s SCORE=%d PELLETS_EATEN=%d PELLETS_TOTAL=%d "
                 "TICKS=%d LIVES=%d LEVEL=%d SEED=%u GHOSTS=%d ANTIFOLLOW=%d "
                 "DIFFICULTY=%d\n",
            status_name(status), g->score, g->pelletsEatenTotal, g->pelletsTotal,
            g->tick, g->lives, g->level, g->rngSeed, g->ghostCount,
            g->antiFollowFlips, g->difficulty);
}
