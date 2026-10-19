/*
 * rng.h - deterministic pseudo-random number generator.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing), shared with Part 3 (Ghost Logic).
 *
 * The assignment requires reproducible runs ("Randomness ... should be
 * reproducible by allowing a user-specified seed").  The C library rand()
 * is NOT used: its sequence differs between platforms and C libraries, so a
 * fixed seed would not reproduce the same game on the marker's machine.
 * xorshift32 is used instead - deterministic, portable and tiny.
 */
#ifndef RNG_H
#define RNG_H

#include <stdint.h>

struct Rng {
    uint32_t state;
};

/* Seed the generator.  Seed 0 is remapped internally (xorshift32 cannot
 * be seeded with 0 because the state would stay 0 forever). */
void rng_seed(struct Rng *rng, uint32_t seed);

/* Next raw 32-bit value of the xorshift32 sequence. */
uint32_t rng_next(struct Rng *rng);

/* Uniformly distributed value in [0, bound).  bound == 0 returns 0.
 * Modulo bias is removed with the standard rejection method. */
uint32_t rng_range(struct Rng *rng, uint32_t bound);

#endif /* RNG_H */
