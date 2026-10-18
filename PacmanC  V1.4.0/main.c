/*
 * main.c - command line handling and the tick loop.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing) owns main.c; the input handling it calls
 * belongs to Part 2.
 *
 * Build (as suggested by the brief):
 *     gcc *.c -o pacman
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "map.h"
#include "render.h"
#include "input.h"
#include "platform.h"
#include "screen.h"

#define PROGRAM_VERSION "1.0"

#define EXIT_OK     0
#define EXIT_USAGE  2
#define EXIT_MAP    3

static void usage(FILE *out, const char *prog)
{
    fprintf(out,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  --seed N        random seed, same seed = same game (default %u)\n"
        "  --ghosts N      1..4 ghosts (default %d)\n"
        "  --levels N      1..%d levels (default %d, extension)\n"
        "  --difficulty N  1..%d, skip the start menu (see README for the table)\n"
        "  --map FILE      load a map from a text file (extension)\n"
        "  --ai MODE       random (default) | pure | smart\n"
        "  --follow-ticks N  anti-follow rule: turn a ghost around after N ticks\n"
        "                    of tailgating (default %d, 1..50)\n"
        "  --follow-range N  how many cells behind are still counted (default %d)\n"
        "  --no-antifollow switch the anti-follow rule off\n"
        "  --core          disable every optional extension\n"
        "  --max-ticks N   stop after N ticks (0 = unlimited)\n"
        "  --quiet         do not print the board on every tick\n"
        "  --plain         plain ASCII display: no frame, no colour (this is the\n"
        "                  output the recorded test logs are compared against)\n"
        "  --check-map F   validate a map file and exit\n"
        "  --check-maps    validate every embedded level and exit\n"
        "  --help, -h      this text\n"
        "  --version       print the version and exit\n"
        "\n"
        "Keys: w=up  s=down  a=left  d=right  q=quit (press Enter after the key)\n"
        "\n"
        "Exit codes: 0 normal, 2 bad usage, 3 map error\n",
        prog, DEFAULT_SEED, DEFAULT_GHOSTS, MAX_LEVELS, DEFAULT_LEVELS,
        DIFFICULTY_MAX, FOLLOW_TICKS, FOLLOW_RANGE);
}

/* Matches "--name value" and "--name=value"; advances *i past the value. */
static int take_value(int argc, char **argv, int *i, const char *name,
                      const char **value)
{
    const char *arg = argv[*i];
    size_t nlen = strlen(name);

    if (strncmp(arg, name, nlen) != 0) {
        return 0;
    }
    if (arg[nlen] == '=') {
        *value = arg + nlen + 1;
        return 1;
    }
    if (arg[nlen] == '\0') {
        if (*i + 1 >= argc) {
            fprintf(stderr, "error: option %s needs a value\n", name);
            exit(EXIT_USAGE);
        }
        *i = *i + 1;
        *value = argv[*i];
        return 1;
    }
    return 0;
}

static long to_long(const char *text, long fallback)
{
    char *end = NULL;
    long v = strtol(text, &end, 10);

    if (end == text || *end != '\0') {
        return fallback;
    }
    return v;
}

/*
 * ---------------------------------------------------------------- menu
 * Ask for the difficulty at the very start (1..5), or q / end of input to
 * leave.  Everything that is not 1..5 and not q is ignored and asked again -
 * the brief asks for input validation and for "do not crash on EOF".
 * Returns 1..5, or 0 when the player quits or the input ends.
 */
static int ask_difficulty(FILE *in, FILE *out)
{
    char line[64];

    for (;;) {
        char *p;

        fprintf(out, "Choose difficulty 1-5 (q to quit): ");
        fflush(out);

        if (fgets(line, (int)sizeof line, in) == NULL) {
            fprintf(out, "\nEnd of input - exiting.\n");
            return 0;                       /* EOF must not crash */
        }

        p = line;
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }

        if (*p == 'q' || *p == 'Q') {
            fprintf(out, "\n");
            return 0;
        }
        if (*p >= '1' && *p <= '5' &&
            (p[1] == '\0' || isspace((unsigned char)p[1]))) {
            fprintf(out, "\n");             /* keep the banner on its own line */
            return *p - '0';
        }

        /* anything else: ignored, ask again */
        fprintf(out, "Invalid input - type a number from 1 to 5, or q to quit.\n");
    }
}

/* Validate every embedded map and print a small report (used by the report's
 * test evidence and handy for the marker). */
static int check_embedded_maps(void)
{
    int i;
    int bad = 0;
    int count = map_embedded_count();

    printf("%-54s %4s %4s %7s %6s  %s\n",
           "embedded map", "rows", "cols", "pellets", "ghosts", "status");
    for (i = 1; i <= count; i++) {
        struct Game g;
        char err[MAP_ERR_LEN];

        memset(&g, 0, sizeof g);
        game_init(&g, DEFAULT_SEED, MAX_GHOSTS, 1, FEAT_ALL, AI_RANDOM);
        g.verbose = 0;

        if (game_load_level(&g, i) != 0) {
            printf("level %d: load error\n", i);
            bad++;
            continue;
        }
        if (map_check(&g, err, (int)sizeof err) != 0) {
            printf("level %d: %s\n", i, err);
            bad++;
            continue;
        }
        printf("%-54s %4d %4d %7d %6d  OK\n",
               g.mapName, g.rows, g.cols, g.pelletsTotal, g.ghostCount);
    }
    if (bad == 0) {
        printf("all %d embedded maps are playable\n", count);
        return EXIT_OK;
    }
    printf("%d of %d embedded maps are broken\n", bad, count);
    return EXIT_MAP;
}

static int check_map_file(const char *path){
    struct Game g;
    char err[MAP_ERR_LEN];

    memset(&g, 0, sizeof g);
    game_init(&g, DEFAULT_SEED, DEFAULT_GHOSTS, 1, FEAT_ALL, AI_RANDOM);

    if (map_load_file(&g, path, err, (int)sizeof err) != 0) {
        printf("map error : %s\n", err);
        return EXIT_MAP;
    }
    if (map_check(&g, err, (int)sizeof err) != 0) {
        printf("map error : %s\n", err);
        return EXIT_MAP;
    }

    printf("map OK    : %s\n", path);
    printf("  size        : %d rows x %d columns\n", g.rows, g.cols);
    printf("  pellets     : %d (all reachable from Pac-Man)\n", g.pelletsTotal);
    printf("  ghost cells : %d\n", g.ghostCount);
    printf("  Pac-Man     : row %d, column %d\n", g.pac.start.r, g.pac.start.c);
    return EXIT_OK;
}

int main(int argc, char **argv)
{
    struct Game game;
    enum GameStatus status = GAME_RUNNING;
    char err[MAP_ERR_LEN];

    unsigned int seed = DEFAULT_SEED;
    int ghosts = DEFAULT_GHOSTS;
    int levels = DEFAULT_LEVELS;
    int levelsExplicit = 0;
    int ghostsExplicit = 0;
    int difficulty = 0;                  /* 0 = not chosen yet (ask)   */
    int flags = FEAT_ALL;
    int aiMode = AI_RANDOM;
    int followTicks = FOLLOW_TICKS;
    int followRange = FOLLOW_RANGE;
    int followExplicit = 0;
    const char *mapFile = NULL;
    long maxTicks = 0;
    int quiet = 0;
    int plain = 0;
    int i;

    for (i = 1; i < argc; i++) {
        const char *val = NULL;
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            usage(stdout, argv[0]);
            return EXIT_OK;
        }
        if (strcmp(arg, "--version") == 0) {
            printf("pacman %s (MFE113TC group project)\n", PROGRAM_VERSION);
            return EXIT_OK;
        }
        if (strcmp(arg, "--quiet") == 0) {
            quiet = 1;
            continue;
        }
        if (strcmp(arg, "--plain") == 0) {
            plain = 1;                       /* no frame, no colour    */
            continue;
        }
        if (strcmp(arg, "--core") == 0) {
            flags = 0;                       /* core requirements only */
            continue;
        }
        if (strcmp(arg, "--no-antifollow") == 0) {
            followTicks = 0;                 /* switch the rule off   */
            continue;
        }
        if (take_value(argc, argv, &i, "--follow-ticks", &val)) {
            followTicks = (int)to_long(val, FOLLOW_TICKS);
            followExplicit = 1;
            if (followTicks < 1 || followTicks > 50) {
                fprintf(stderr, "error: --follow-ticks must be between 1 and 50\n");
                return EXIT_USAGE;
            }
            continue;
        }
        if (take_value(argc, argv, &i, "--follow-range", &val)) {
            followRange = (int)to_long(val, FOLLOW_RANGE);
            if (followRange < 1 || followRange > 10) {
                fprintf(stderr, "error: --follow-range must be between 1 and 10\n");
                return EXIT_USAGE;
            }
            continue;
        }
        if (take_value(argc, argv, &i, "--difficulty", &val)) {
            difficulty = (int)to_long(val, 0);
            if (difficulty < DIFFICULTY_MIN || difficulty > DIFFICULTY_MAX) {
                fprintf(stderr, "error: --difficulty must be between %d and %d\n",
                        DIFFICULTY_MIN, DIFFICULTY_MAX);
                return EXIT_USAGE;
            }
            continue;
        }
        if (take_value(argc, argv, &i, "--seed", &val)) {
            seed = (unsigned int)to_long(val, (long)DEFAULT_SEED);
            continue;
        }
        if (take_value(argc, argv, &i, "--ghosts", &val)) {
            ghosts = (int)to_long(val, DEFAULT_GHOSTS);
            ghostsExplicit = 1;
            if (ghosts < 1 || ghosts > MAX_GHOSTS) {
                fprintf(stderr, "error: --ghosts must be between 1 and %d\n", MAX_GHOSTS);
                return EXIT_USAGE;
            }
            continue;
        }
        if (take_value(argc, argv, &i, "--levels", &val)) {
            levels = (int)to_long(val, DEFAULT_LEVELS);
            levelsExplicit = 1;
            if (levels < 1 || levels > MAX_LEVELS) {
                fprintf(stderr, "error: --levels must be between 1 and %d\n", MAX_LEVELS);
                return EXIT_USAGE;
            }
            continue;
        }
        if (take_value(argc, argv, &i, "--map", &val)) {
            mapFile = val;
            continue;
        }
        if (take_value(argc, argv, &i, "--ai", &val)) {
            if (strcmp(val, "random") == 0) {
                aiMode = AI_RANDOM;
            } else if (strcmp(val, "pure") == 0) {
                aiMode = AI_PURE;
            } else if (strcmp(val, "smart") == 0) {
                aiMode = AI_SMART;
            } else {
                fprintf(stderr, "error: --ai must be random, pure or smart\n");
                return EXIT_USAGE;
            }
            continue;
        }
        if (take_value(argc, argv, &i, "--max-ticks", &val)) {
            maxTicks = to_long(val, 0);
            continue;
        }
        if (take_value(argc, argv, &i, "--check-map", &val)) {
            return check_map_file(val);
        }
        if (strcmp(arg, "--check-maps") == 0) {
            return check_embedded_maps();
        }

        fprintf(stderr, "error: unknown option '%s'\n", arg);
        usage(stderr, argv[0]);
        return EXIT_USAGE;
    }

    /* ------------------------------------------------------------- display
     * The default display is the framed, coloured one.  --plain keeps the
     * original ASCII output byte for byte, which is what the recorded test
     * logs were produced with.
     */
    if (!plain) {
        platform_init_console();
        screen_clear_terminal(stdout);
        render_title(stdout, 1);
    }

    /* ---------------------------------------------------------------- menu
     * The start menu is only skipped when --difficulty was given, so scripted
     * runs (tests, pipes) never have to answer it.
     */
    if (difficulty == 0) {
        difficulty = ask_difficulty(stdin, stdout);
        if (difficulty == 0) {
            if (!plain) {
                platform_restore_console();
            }
            printf("No game started.\n");
            return EXIT_OK;
        }
    }

    /* the difficulty table provides the defaults; explicit options win */
    {
        int tableLevels = levels;
        int tableGhosts = ghosts;

        game_difficulty(difficulty, &tableLevels, &tableGhosts);
        if (!levelsExplicit) {
            levels = tableLevels;
        }
        if (!ghostsExplicit) {
            ghosts = tableGhosts;
        }
    }

    /* a map file contains exactly one level unless --levels says otherwise */
    if (mapFile != NULL && !levelsExplicit) {
        levels = 1;
    }
    if (!(flags & FEAT_LEVELS)) {
        levels = 1;
    }

    game_init(&game, seed, ghosts, levels, flags, aiMode);
    game.verbose = plain ? 1 : 0;   /* fancy display shows events in its footer */

    /* --core means "exactly the requirements of the brief": the anti-follow
     * rule is our own refinement, so it is off unless it was asked for */
    if (flags == 0 && !followExplicit) {
        followTicks = 0;
    }
    game.followLimit = followTicks;
    game.followRange = followRange;
    game.difficulty = difficulty;

    if (mapFile != NULL) {
        if (map_load_file(&game, mapFile, err, (int)sizeof err) != 0 ||
            map_check(&game, err, (int)sizeof err) != 0) {
            fprintf(stderr, "map error: %s\n", err);
            return EXIT_MAP;
        }
        game.level = 1;
        if (game.ghostCount > game.ghostRequested) {
            game.ghostCount = game.ghostRequested;
        }
        if (plain) {
            printf("Loaded map file '%s': %d rows x %d columns, %d pellets, "
                   "%d ghost cell(s)\n",
                   mapFile, game.rows, game.cols, game.pelletsTotal, game.ghostCount);
        }
    } else {
        if (game_load_level(&game, 1) != 0) {
            return EXIT_MAP;
        }
    }

    if (plain) {
        render_banner(&game, stdout);
    }
    if (!quiet) {
        if (plain) {
            render_all(&game, stdout);
        } else {
            render_frame(&game, stdout, 1);
        }
    }

    while (status == GAME_RUNNING) {
        struct Input in = input_read(stdin);

        if (in.kind == INPUT_EOF) {
            printf("\nEnd of input (EOF) reached - exiting without crashing.\n");
            status = GAME_QUIT;
            break;
        }
        if (in.kind == INPUT_QUIT) {
            printf("\nQuit requested.\n");
            status = GAME_QUIT;
            break;
        }
        if (in.kind == INPUT_INVALID) {
            if (plain) {
                printf("Unknown key '%c' - use w/a/s/d to move or q to quit.\n",
                       in.key);
            } else {
                snprintf(game.lastEvent, sizeof game.lastEvent,
                         "Unknown key '%c' - use w/a/s/d to move or q to quit",
                         in.key);
                render_frame(&game, stdout, 1);
            }
            continue;                        /* invalid keys never cost a tick */
        }

        status = game_tick(&game, in.dr, in.dc);

        if (!quiet) {
            if (plain) {
                render_all(&game, stdout);
            } else {
                render_frame(&game, stdout, 1);
            }
        }

        if (plain) {
            if (status == GAME_WIN) {
                printf("\n*** All pellets eaten - YOU WIN! ***\n");
            } else if (status == GAME_LOSE) {
                printf("\n*** No lives left - GAME OVER ***\n");
            }
        }

        if (maxTicks > 0 && game.tick >= maxTicks && status == GAME_RUNNING) {
            if (plain) {
                printf("\nTick limit of %ld reached - stopping.\n", maxTicks);
            }
            status = GAME_QUIT;
        }
    }

    if (!plain) {
        platform_restore_console();
        printf("\n");
    }
    game_print_summary(&game, status, stdout);
    return EXIT_OK;
}
