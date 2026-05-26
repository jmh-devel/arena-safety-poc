#include "arena.h"

void arena_init(Arena *arena, void *buffer, size_t capacity)
{
    if (arena == 0) {
        return;
    }

    arena->buffer = (unsigned char *)buffer;
    arena->capacity = capacity;
    arena->offset = 0;
}

void arena_clear(Arena *arena)
{
    if (arena != 0) {
        arena->offset = 0;
    }
}

void *arena_vulnerable_alloc(Arena *arena, size_t size, size_t alignment)
{
    uintptr_t base;
    uintptr_t current;
    uintptr_t aligned;
    size_t new_offset;

    if (arena == 0 || arena->buffer == 0 || alignment == 0) {
        return 0;
    }

    base = (uintptr_t)arena->buffer;
    current = base + arena->offset;
    aligned = (current + (alignment - 1U)) & ~(uintptr_t)(alignment - 1U);
    new_offset = (size_t)(aligned - base) + size;

    if (new_offset > arena->capacity) {
        return 0;
    }

    arena->offset = new_offset;
    return (void *)aligned;
}

