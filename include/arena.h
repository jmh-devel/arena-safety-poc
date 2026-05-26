#ifndef ARENA_POC_ARENA_H
#define ARENA_POC_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct Arena {
    unsigned char *buffer;
    size_t capacity;
    size_t offset;
} Arena;

typedef enum ArenaStatus {
    ARENA_OK = 0,
    ARENA_ERR_NULL = 1,
    ARENA_ERR_ALIGNMENT = 2,
    ARENA_ERR_OVERFLOW = 3,
    ARENA_ERR_OOM = 4,
    ARENA_ERR_STATE = 5
} ArenaStatus;

void arena_init(Arena *arena, void *buffer, size_t capacity);
void arena_clear(Arena *arena);

void *arena_vulnerable_alloc(Arena *arena, size_t size, size_t alignment);

ArenaStatus arena_hardened_alloc(Arena *arena, size_t size, size_t alignment, void **out);
const char *arena_status_name(ArenaStatus status);

#endif

