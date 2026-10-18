/*
 * rng.c - xorshift32 implementation (see rng.h).
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing).
 */
#include "rng.h"

#define RNG_FALLBACK_SEED 0x9E3779B9u   /* golden-ratio constant */

void rng_seed(struct Rng *rng, uint32_t seed)
{
    rng->state = (seed == 0u) ? RNG_FALLBACK_SEED : seed;
}

uint32_t rng_next(struct Rng *rng)
{
    uint32_t x = rng->state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng->state = x;
    return x;
}

uint32_t rng_range(struct Rng *rng, uint32_t bound)
{
    uint32_t limit;
    uint32_t r;

    if (bound == 0u || bound == 1u) {
        return 0u;
    }

    /* 2^32 mod bound: values below this limit would make the modulo
     * operation slightly biased, so they are rejected. */
    limit = (uint32_t)(0u - bound) % bound;

    do {
        r = rng_next(rng);
    } while (r < limit);

    return r % bound;
}
