/*
 * game.h - shared state and rule constants of the game.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing) owns this file; every other part includes it.
 *
 * The structures follow the "Suggested Data Structures" section of the brief
 * (struct Pos / struct Entity / struct Game).  Extra fields were appended for
 * the optional extensions (power pellets, ghost modes, fruit, levels); they
 * are listed at the end of struct Game so the original layout stays readable.
 */
#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include "rng.h"

/* ----------------------------------------------------------------- limits */
#define MAP_MAX_ROWS      25      /* brief: "up to 25 x 40"            */
#define MAP_MAX_COLS      40
#define MAX_GHOSTS         4      /* brief: G in {1,2,3,4}             */
#define MAX_LEVELS         5      /* five hard-coded maps (extension)  */

/* ------------------------------------------------------- board characters */
#define CELL_WALL    '#'
#define CELL_PELLET  '.'
#define CELL_POWER   'O'
#define CELL_EMPTY   ' '
#define CHAR_PAC     'P'
#define CHAR_GHOST   'G'
#define CHAR_SCARED  '!'          /* extension: frightened ghost       */
#define CHAR_FRUIT   '*'

/* ------------------------------------------------------- rule constants */
/* Every number below is a documented design decision (see report 1.7). */
#define START_LIVES            3
#define PELLET_POINTS         10
#define POWER_PELLET_POINTS   50
#define GHOST_POINTS_BASE    200      /* doubles per ghost in one chain */
#define GHOST_CHAIN_MAX        4
#define FRUIT_POINTS         100
#define SCARED_TICKS          20      /* T in the brief                 */
#define GHOST_RESPAWN_TICKS    5      /* "respawn after a delay"        */
#define CHASE_TICKS           20      /* N in the brief                 */
#define SCATTER_TICKS         10      /* M in the brief                 */
#define FRUIT_FIRST_TICK      30
#define FRUIT_INTERVAL        40
#define FRUIT_LIFETIME        15
#define DEFAULT_GHOSTS         2
#define DEFAULT_LEVELS         2
#define DEFAULT_SEED          1u      /* fixed default => reproducible  */

/* ------------------------------------------------- anti-follow (Part 3) */
/*
 * The brief's optional rule of thumb forbids a ghost to reverse while another
 * direction is open.  A player can therefore stay behind a ghost - at ANY
 * distance, because both move one cell per tick - and can never be caught, so
 * the ghost turns around after Pac-Man has been in the SAME corridor, straight
 * behind it, for FOLLOW_TICKS consecutive ticks (see ghost.c).
 *
 * FOLLOW_RANGE measures the gap, not the adjacency: 5 means "up to four empty
 * cells between Pac-Man and the ghost", which is the distance a player
 * naturally keeps when tailgating.  The corridor test stops at the first wall,
 * so the parallel corridor of a maze never triggers the rule.
 */
#define FOLLOW_RANGE           5      /* cells behind that still count      */
#define FOLLOW_TICKS           3      /* consecutive ticks before it turns  */
#define FOLLOW_COOLDOWN        2      /* ticks before it can trigger again  */

/* ---------------------------------------------------------- difficulty */
/* The start menu offers 1..5; the table in game.c turns the choice into a
 * number of levels and a number of ghosts (see game_difficulty()). */
#define DIFFICULTY_MIN         1
#define DIFFICULTY_MAX         5

/* --------------------------------------------------------- feature flags */
#define FEAT_POWER   0x01     /* power pellets + scared ghosts          */
#define FEAT_MODES   0x02     /* chase / scatter alternation            */
#define FEAT_FRUIT   0x04     /* bonus fruit                            */
#define FEAT_LEVELS  0x08     /* more than one level                    */
#define FEAT_ALL     (FEAT_POWER | FEAT_MODES | FEAT_FRUIT | FEAT_LEVELS)

/* ------------------------------------------------------------ ghost AI */
enum AiMode {
    AI_RANDOM = 0,   /* default: uniform random valid direction, no reverse
                        unless it is the only option (brief section 3)      */
    AI_PURE   = 1,   /* literal reading: uniform over every valid direction  */
    AI_SMART  = 2    /* extension: heuristic chase / scatter                 */
};

enum GhostState {
    GHOST_NORMAL = 0,
    GHOST_SCARED = 1,
    GHOST_EATEN  = 2
};

enum GlobalMode {
    MODE_SCATTER = 0,
    MODE_CHASE   = 1
};

/* ------------------------------------------------------- core structures */
struct Pos {
    int r, c;
};

struct Entity {
    struct Pos pos;      /* current cell                                  */
    struct Pos start;    /* spawn cell (used for respawn)                 */
    int dr, dc;          /* facing / velocity, one cell per tick          */
    int state;           /* enum GhostState (ghosts only)                 */
    int respawnIn;       /* ticks left before an eaten ghost returns      */
    int followTicks;     /* consecutive ticks Pac-Man was following it    */
    int followCooldown;  /* ticks during which the detector is idle       */
    int forceReverse;    /* set by the detector, applied on the next tick */
};

struct Game {
    int rows, cols;
    char board[MAP_MAX_ROWS][MAP_MAX_COLS];

    struct Entity pac;
    struct Entity ghosts[MAX_GHOSTS];
    int ghostCount;

    int score;
    int lives;
    int pelletsRemaining;     /* pellets left on the current level    */
    int pelletsTotal;         /* pellets the current level started with */
    int pelletsEatenTotal;    /* cumulative, all levels               */
    unsigned int rngSeed;     /* the seed requested on the command line */
    struct Rng rng;

    int tick;
    int level;
    int levels;
    int flags;                /* FEAT_* bit mask                     */
    int aiMode;               /* enum AiMode                         */
    int ghostRequested;       /* --ghosts value, clamped to map data  */
    int verbose;              /* 1 = print game events to stdout      */

    /* anti-follow rule (extension, Part 3) */
    int followLimit;          /* ticks of tailgating before a turn-around;
                                 0 switches the rule off               */
    int followRange;          /* how many cells behind still count    */
    int antiFollowFlips;      /* how often the rule fired (evidence)  */

    int difficulty;           /* 1..5 from the start menu (0 = not chosen) */

    /* extension state */
    int scaredTicks;
    int ghostChain;
    int globalMode;
    int modeTimer;
    int fruitActive;
    struct Pos fruitPos;
    int fruitTicksLeft;
    int fruitSpawnIn;

    char mapName[64];
    int mapIsFile;

    /* last thing that happened - shown in the footer of the framed display */
    char lastEvent[96];
};

enum GameStatus {
    GAME_RUNNING = 0,
    GAME_WIN,
    GAME_LOSE,
    GAME_QUIT
};

/* --------------------------------------------------------------- API */
/* Initialise a game: seed, requested ghost count, level count, feature
 * flags and AI mode.  The board is filled when the first level is loaded. */
void game_init(struct Game *g, unsigned int seed, int ghosts, int levels,
               int flags, int aiMode);

/* Load an embedded level (1-based).  Returns 0 on success. */
int game_load_level(struct Game *g, int level);

/* Run exactly one tick with the direction requested by the player.
 * A move into a wall keeps Pac-Man in place (the tick still advances). */
enum GameStatus game_tick(struct Game *g, int dr, int dc);

/* Difficulty table: choice 1..5 -> number of levels and number of ghosts.
 * Values outside the range are clamped into it. */
void game_difficulty(int choice, int *levels, int *ghosts);

/* True while a fruit is on the board. */
int game_fruit_active(const struct Game *g);

/* Human readable + machine readable closing summary. */
void game_print_summary(const struct Game *g, enum GameStatus status, FILE *out);

#endif /* GAME_H */
