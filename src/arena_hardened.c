#include "arena.h"

#include <limits.h>

static int is_power_of_two(size_t value)
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

static int add_size_overflows(size_t left, size_t right, size_t *out)
{
    if (left > SIZE_MAX - right) {
        return 1;
    }

    *out = left + right;
    return 0;
}

static int add_uintptr_overflows(uintptr_t left, uintptr_t right, uintptr_t *out)
{
    if (left > UINTPTR_MAX - right) {
        return 1;
    }

    *out = left + right;
    return 0;
}

ArenaStatus arena_hardened_alloc(Arena *arena, size_t size, size_t alignment, void **out)
{
    uintptr_t base;
    uintptr_t current;
    uintptr_t rounded;
    uintptr_t aligned;
    size_t aligned_offset;
    size_t new_offset;

    if (out != 0) {
        *out = 0;
    }

    if (arena == 0 || out == 0 || arena->buffer == 0) {
        return ARENA_ERR_NULL;
    }

    if (!is_power_of_two(alignment)) {
        return ARENA_ERR_ALIGNMENT;
    }

    if (arena->offset > arena->capacity) {
        return ARENA_ERR_STATE;
    }

    base = (uintptr_t)arena->buffer;

    if (add_uintptr_overflows(base, (uintptr_t)arena->offset, &current)) {
        return ARENA_ERR_OVERFLOW;
    }

    if (add_uintptr_overflows(current, (uintptr_t)(alignment - 1U), &rounded)) {
        return ARENA_ERR_OVERFLOW;
    }

    aligned = rounded & ~(uintptr_t)(alignment - 1U);

    if (aligned < base) {
        return ARENA_ERR_OVERFLOW;
    }

    aligned_offset = (size_t)(aligned - base);
    if (aligned_offset < arena->offset || aligned_offset > arena->capacity) {
        return ARENA_ERR_STATE;
    }

    if (add_size_overflows(aligned_offset, size, &new_offset)) {
        return ARENA_ERR_OVERFLOW;
    }

    if (new_offset > arena->capacity) {
        return ARENA_ERR_OOM;
    }

    arena->offset = new_offset;
    *out = (void *)aligned;
    return ARENA_OK;
}

const char *arena_status_name(ArenaStatus status)
{
    switch (status) {
    case ARENA_OK:
        return "ok";
    case ARENA_ERR_NULL:
        return "null";
    case ARENA_ERR_ALIGNMENT:
        return "alignment";
    case ARENA_ERR_OVERFLOW:
        return "overflow";
    case ARENA_ERR_OOM:
        return "oom";
    case ARENA_ERR_STATE:
        return "state";
    case ARENA_ERR_SYSTEM:
        return "system";
    default:
        return "unknown";
    }
}
