#include "arena.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

static void test_normal_allocation(void)
{
    unsigned char storage[128];
    Arena arena;
    void *first;
    void *second;
    void *safe_first;
    void *safe_second;

    arena_init(&arena, storage, sizeof(storage));
    first = arena_vulnerable_alloc(&arena, 7, 8);
    second = arena_vulnerable_alloc(&arena, 16, 16);
    CHECK(first != 0);
    CHECK(second != 0);
    CHECK(((uintptr_t)first % 8U) == 0U);
    CHECK(((uintptr_t)second % 16U) == 0U);
    CHECK(arena.offset <= sizeof(storage));

    arena_clear(&arena);
    CHECK(arena.offset == 0U);

    CHECK(arena_hardened_alloc(&arena, 7, 8, &safe_first) == ARENA_OK);
    CHECK(arena_hardened_alloc(&arena, 16, 16, &safe_second) == ARENA_OK);
    CHECK(safe_first != 0);
    CHECK(safe_second != 0);
    CHECK(((uintptr_t)safe_first % 8U) == 0U);
    CHECK(((uintptr_t)safe_second % 16U) == 0U);
}

static void test_non_power_of_two_alignment(void)
{
    unsigned char storage[128];
    Arena arena;
    void *vulnerable;
    void *hardened;

    arena_init(&arena, storage, sizeof(storage));
    vulnerable = arena_vulnerable_alloc(&arena, 1, 24);
    CHECK(vulnerable != 0);

    arena_clear(&arena);
    CHECK(arena_hardened_alloc(&arena, 1, 24, &hardened) == ARENA_ERR_ALIGNMENT);
    CHECK(hardened == 0);
    CHECK(arena.offset == 0U);
}

static void test_uintptr_wraparound(void)
{
    Arena arena;
    void *vulnerable;
    void *hardened;
    const size_t hostile_offset = SIZE_MAX - 3U;

    arena_init(&arena, (void *)(uintptr_t)0x1000U, SIZE_MAX);
    arena.offset = hostile_offset;

    vulnerable = arena_vulnerable_alloc(&arena, 8, 16);
    CHECK(vulnerable != 0);
    CHECK((uintptr_t)vulnerable < (uintptr_t)arena.buffer || arena.offset < hostile_offset);

    arena_init(&arena, (void *)(uintptr_t)0x1000U, SIZE_MAX);
    arena.offset = hostile_offset;
    CHECK(arena_hardened_alloc(&arena, 8, 16, &hardened) == ARENA_ERR_OVERFLOW);
    CHECK(hardened == 0);
    CHECK(arena.offset == hostile_offset);
}

static void test_size_wraparound(void)
{
    Arena arena;
    void *vulnerable;
    void *hardened;

    arena_init(&arena, (void *)(uintptr_t)0x1000U, SIZE_MAX);
    arena.offset = 32;

    vulnerable = arena_vulnerable_alloc(&arena, SIZE_MAX - 16U, 16);
    CHECK(vulnerable != 0);
    CHECK(arena.offset < 32U);

    arena_init(&arena, (void *)(uintptr_t)0x1000U, SIZE_MAX);
    arena.offset = 32;
    CHECK(arena_hardened_alloc(&arena, SIZE_MAX - 16U, 16, &hardened) == ARENA_ERR_OVERFLOW);
    CHECK(hardened == 0);
    CHECK(arena.offset == 32U);
}

static void test_out_of_memory(void)
{
    unsigned char storage[16];
    Arena arena;
    void *hardened;

    arena_init(&arena, storage, sizeof(storage));
    CHECK(arena_vulnerable_alloc(&arena, 32, 8) == 0);
    CHECK(arena.offset == 0U);

    CHECK(arena_hardened_alloc(&arena, 32, 8, &hardened) == ARENA_ERR_OOM);
    CHECK(hardened == 0);
    CHECK(arena.offset == 0U);
}

static void test_zero_size_is_rejected(void)
{
    unsigned char storage[16];
    Arena arena;
    void *first;
    void *second;

    arena_init(&arena, storage, sizeof(storage));
    CHECK(arena_hardened_alloc(&arena, 0, 8, &first) == ARENA_ERR_SIZE);
    CHECK(first == 0);
    CHECK(arena.offset == 0U);

    CHECK(arena_hardened_alloc(&arena, 0, 8, &second) == ARENA_ERR_SIZE);
    CHECK(second == 0);
    CHECK(arena.offset == 0U);
}

static void test_padding_can_exhaust_capacity(void)
{
    unsigned char storage[64];
    Arena arena;
    void *hardened;

    arena_init(&arena, storage, 8);
    arena.offset = 1;

    CHECK(arena_hardened_alloc(&arena, 1, 64, &hardened) == ARENA_ERR_OOM);
    CHECK(hardened == 0);
    CHECK(arena.offset == 1U);
}

static void test_bad_arguments_do_not_mutate_state(void)
{
    unsigned char storage[16];
    Arena arena;
    void *stale = storage;

    arena_init(&arena, storage, sizeof(storage));

    CHECK(arena_hardened_alloc(0, 1, 1, &stale) == ARENA_ERR_NULL);
    CHECK(stale == 0);

    stale = storage;
    CHECK(arena_hardened_alloc(&arena, 1, 1, 0) == ARENA_ERR_NULL);
    CHECK(stale == storage);
    CHECK(arena.offset == 0U);

    arena_init(&arena, 0, sizeof(storage));
    stale = storage;
    CHECK(arena_hardened_alloc(&arena, 1, 1, &stale) == ARENA_ERR_NULL);
    CHECK(stale == 0);
    CHECK(arena.offset == 0U);
}

static void test_corrupted_offset_is_rejected(void)
{
    unsigned char storage[16];
    Arena arena;
    void *hardened;

    arena_init(&arena, storage, sizeof(storage));
    arena.offset = sizeof(storage) + 1U;

    CHECK(arena_hardened_alloc(&arena, 1, 1, &hardened) == ARENA_ERR_STATE);
    CHECK(hardened == 0);
    CHECK(arena.offset == sizeof(storage) + 1U);
}

int main(void)
{
    test_normal_allocation();
    test_non_power_of_two_alignment();
    test_uintptr_wraparound();
    test_size_wraparound();
    test_out_of_memory();
    test_zero_size_is_rejected();
    test_padding_can_exhaust_capacity();
    test_bad_arguments_do_not_mutate_state();
    test_corrupted_offset_is_rejected();

    puts("arena-safety-poc: all tests passed");
    return 0;
}
