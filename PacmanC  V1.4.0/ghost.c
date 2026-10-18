/*
 * ghost.c - ghost behaviour.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 3 (Ghost Logic).
 *
 * Core rule (brief section 3): every tick each ghost picks a uniformly
 * random direction among {up, down, left, right} that is not a wall; the
 * optional rule of thumb "avoid reversing direction unless no alternative"
 * is enabled by default because the brief recommends it (it removes the
 * jitter of a ghost oscillating between two cells).
 *
 * The optional smart mode reuses the same candidate list but selects the
 * direction heuristically (chase / scatter / flee).
 */
#include <stdio.h>
#include "ghost.h"
#include "entity.h"
#include "map.h"

static const int DIRS[4][2] = {
    { -1, 0 },   /* up    */
    {  1, 0 },   /* down  */
    {  0, -1 },  /* left  */
    {  0,  1 }   /* right */
};

int ghost_valid_dir_count(const struct Game *g, const struct Entity *gh)
{
    int i;
    int n = 0;

    for (i = 0; i < 4; i++) {
        if (cell_is_open(g, gh->pos.r + DIRS[i][0], gh->pos.c + DIRS[i][1])) {
            n++;
        }
    }
    return n;
}

void ghost_update_global_mode(struct Game *g)
{
    if (g->modeTimer > 0) {
        g->modeTimer--;
    }
    if (g->modeTimer == 0) {
        if (g->globalMode == MODE_SCATTER) {
            g->globalMode = MODE_CHASE;
            g->modeTimer = CHASE_TICKS;
        } else {
            g->globalMode = MODE_SCATTER;
            g->modeTimer = SCATTER_TICKS;
        }
    }
}

void ghost_reset_all(struct Game *g)
{
    int i;

    for (i = 0; i < g->ghostCount; i++) {
        g->ghosts[i].pos = g->ghosts[i].start;
        g->ghosts[i].dr = 0;
        g->ghosts[i].dc = 0;
        g->ghosts[i].state = GHOST_NORMAL;
        g->ghosts[i].respawnIn = 0;
        g->ghosts[i].followTicks = 0;
        g->ghosts[i].followCooldown = 0;
        g->ghosts[i].forceReverse = 0;
    }
}

/*
 * ------------------------------------------------------- anti-follow rule
 *
 * The brief's optional rule of thumb ("do not reverse unless there is no
 * alternative") has a side effect: a player who stays one cell behind a ghost
 * can never be caught, because the ghost is not allowed to turn round.  The
 * detector below recognises exactly that situation - Pac-Man in the SAME
 * corridor, directly behind the ghost, at most FOLLOW_RANGE cells away, with no
 * wall between - and turns the ghost round after FOLLOW_TICKS consecutive
 * ticks.  A ghost that is only 2 cells behind a wall (the parallel corridor of
 * a maze) is never affected, because the corridor test stops at the first wall.
 */
static int pac_is_tailgating(const struct Game *g, const struct Entity *gh)
{
    int i;
    int r, c;

    if (g->followLimit <= 0) {
        return 0;                       /* rule switched off (--no-antifollow) */
    }
    if (gh->state != GHOST_NORMAL) {
        return 0;                       /* chasing a scared ghost must work    */
    }
    if (!dir_is_unit(gh->dr, gh->dc)) {
        return 0;                       /* has not moved yet                   */
    }

    for (i = 1; i <= g->followRange; i++) {
        r = gh->pos.r - gh->dr * i;     /* straight behind the ghost           */
        c = gh->pos.c - gh->dc * i;
        if (!cell_is_open(g, r, c)) {
            return 0;                   /* wall first: nobody follows from there */
        }
        if (g->pac.pos.r == r && g->pac.pos.c == c) {
            return 1;                   /* same corridor, behind, no wall      */
        }
    }
    return 0;
}

static void update_follow_counter(struct Game *g, int index)
{
    struct Entity *gh = &g->ghosts[index];

    if (g->followLimit <= 0) {
        gh->followTicks = 0;
        return;
    }
    if (gh->followCooldown > 0) {
        gh->followCooldown--;
        gh->followTicks = 0;
        return;
    }
    if (pac_is_tailgating(g, gh)) {
        gh->followTicks++;
    } else {
        gh->followTicks = 0;
    }

    if (gh->followTicks >= g->followLimit) {
        gh->forceReverse = 1;
        gh->followTicks = 0;
        gh->followCooldown = FOLLOW_COOLDOWN;
        g->antiFollowFlips++;
        if (g->verbose) {
            printf("  * Ghost %d turns around - Pac-Man was following it\n",
                   index + 1);
        }
    }
}

/* Home corner used by the scatter mode - one corner per ghost. */
static struct Pos corner_of(const struct Game *g, int index)
{
    struct Pos p;

    switch (index % 4) {
    case 0:  p.r = 1;              p.c = 1;              break;
    case 1:  p.r = 1;              p.c = g->cols - 2;    break;
    case 2:  p.r = g->rows - 2;    p.c = 1;              break;
    default: p.r = g->rows - 2;    p.c = g->cols - 2;    break;
    }
    return p;
}

/*
 * Heuristic choice (extension): pick the candidate cell that is closest to
 * the target (chase), closest to the home corner (scatter) or farthest from
 * Pac-Man (scared).  Ties are broken with the RNG so the ghosts still look
 * alive, while the run stays reproducible for a fixed seed.
 */
static int heuristic_choice(struct Game *g, int index, int cand[4][2], int n)
{
    struct Pos target;
    int farthest = 0;
    long best = 0;
    int bestIdx[4];
    int bestCount = 0;
    int i;

    if (g->ghosts[index].state == GHOST_SCARED) {
        target = g->pac.pos;
        farthest = 1;
    } else if (g->globalMode == MODE_CHASE) {
        target = g->pac.pos;
    } else {
        target = corner_of(g, index);
    }

    for (i = 0; i < n; i++) {
        long dr = (long)(g->ghosts[index].pos.r + cand[i][0]) - (long)target.r;
        long dc = (long)(g->ghosts[index].pos.c + cand[i][1]) - (long)target.c;
        long dist = dr * dr + dc * dc;

        if (bestCount == 0 ||
            (farthest ? (dist > best) : (dist < best))) {
            best = dist;
            bestIdx[0] = i;
            bestCount = 1;
        } else if (dist == best) {
            bestIdx[bestCount] = i;
            bestCount++;
        }
    }

    return bestIdx[(int)rng_range(&g->rng, (uint32_t)bestCount)];
}

void ghost_update(struct Game *g, int index)
{
    struct Entity *gh = &g->ghosts[index];
    int cand[4][2];
    int n = 0;
    int i;
    int chosen;
    int cur_dr;
    int cur_dc;

    /* an eaten ghost is off the board until its delay has passed */
    if (gh->state == GHOST_EATEN) {
        gh->followTicks = 0;
        if (gh->respawnIn > 0) {
            gh->respawnIn--;
        }
        if (gh->respawnIn == 0) {
            gh->pos = gh->start;
            gh->dr = 0;
            gh->dc = 0;
            gh->state = GHOST_NORMAL;
        }
        return;
    }

    /* a pending turn-around is applied before the direction is chosen, so the
     * normal "no reversal" filter keeps working afterwards */
    if (gh->forceReverse) {
        gh->forceReverse = 0;
        if (dir_is_unit(gh->dr, gh->dc)) {
            gh->dr = -gh->dr;
            gh->dc = -gh->dc;
        }
    }

    /* the facing AFTER a possible turn-around is the one the filter uses */
    cur_dr = gh->dr;
    cur_dc = gh->dc;


    /* 1 - every legal direction (never a wall) */
    for (i = 0; i < 4; i++) {
        if (cell_is_open(g, gh->pos.r + DIRS[i][0], gh->pos.c + DIRS[i][1])) {
            cand[n][0] = DIRS[i][0];
            cand[n][1] = DIRS[i][1];
            n++;
        }
    }

    if (n == 0) {
        /* completely walled in: the ghost simply stays where it is */
        gh->dr = 0;
        gh->dc = 0;
        return;
    }

    /* 2 - rule of thumb: do not reverse unless there is no alternative */
    if (g->aiMode != AI_PURE && n > 1 && (cur_dr != 0 || cur_dc != 0)) {
        int m = 0;

        for (i = 0; i < n; i++) {
            if (!(cand[i][0] == -cur_dr && cand[i][1] == -cur_dc)) {
                cand[m][0] = cand[i][0];
                cand[m][1] = cand[i][1];
                m++;
            }
        }
        if (m > 0) {
            n = m;
        }
    }

    /* 3 - choose: uniform random (core) or heuristic (extension) */
    if (g->aiMode == AI_SMART) {
        chosen = heuristic_choice(g, index, cand, n);
    } else {
        chosen = (int)rng_range(&g->rng, (uint32_t)n);
    }

    gh->dr = cand[chosen][0];
    gh->dc = cand[chosen][1];

    (void)entity_step(g, gh);

    /* 4 - anti-follow rule: count how long Pac-Man has been following it */
    update_follow_counter(g, index);
}
