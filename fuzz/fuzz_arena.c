#include "arena.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t xorshift64(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

int main(int argc, char **argv)
{
    unsigned char storage[4096];
    unsigned long iterations = 10000;
    uint64_t seed = 0xC05F00DULL;
    unsigned long rejected = 0;
    unsigned long accepted = 0;

    if (argc > 1) {
        iterations = strtoul(argv[1], 0, 10);
    }

    for (unsigned long i = 0; i < iterations; i++) {
        Arena arena;
        void *ptr;
        size_t size = (size_t)(xorshift64(&seed) % 512U);
        size_t alignment = (size_t)(xorshift64(&seed) % 128U);
        ArenaStatus status;

        arena_init(&arena, storage, sizeof(storage));
        arena.offset = (size_t)(xorshift64(&seed) % sizeof(storage));

        status = arena_hardened_alloc(&arena, size, alignment, &ptr);
        if (status == ARENA_OK) {
            accepted++;
            if (ptr == 0 || ((uintptr_t)ptr % alignment) != 0U || arena.offset > arena.capacity) {
                fprintf(stderr, "contract violation at iteration %lu\n", i);
                return 1;
            }
        } else {
            rejected++;
        }
    }

    printf("fuzz complete: accepted=%lu rejected=%lu\n", accepted, rejected);
    return 0;
}

