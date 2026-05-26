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
    ARENA_ERR_STATE = 5,
    ARENA_ERR_SYSTEM = 6,
    ARENA_ERR_SIZE = 7
} ArenaStatus;

typedef struct ArenaMapping {
    Arena arena;
    void *mapping;
    size_t mapped_size;
    size_t usable_size;
    size_t page_size;
} ArenaMapping;

void arena_init(Arena *arena, void *buffer, size_t capacity);
void arena_clear(Arena *arena);

void *arena_vulnerable_alloc(Arena *arena, size_t size, size_t alignment);

ArenaStatus arena_hardened_alloc(Arena *arena, size_t size, size_t alignment, void **out);
const char *arena_status_name(ArenaStatus status);

ArenaStatus arena_mmap_create(ArenaMapping *mapping, size_t usable_size, int guard_page);
void arena_mmap_destroy(ArenaMapping *mapping);

#endif
