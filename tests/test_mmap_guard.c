#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "arena.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

static size_t page_size(void)
{
    long value = sysconf(_SC_PAGESIZE);
    CHECK(value > 0);
    return (size_t)value;
}

static void *map_guarded(size_t usable_size, size_t *mapped_size)
{
    size_t page = page_size();
    size_t usable_pages = (usable_size + page - 1U) / page;
    size_t usable_rounded = usable_pages * page;
    size_t total = usable_rounded + page;
    unsigned char *mapping = mmap(0, total, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mapping == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        exit(1);
    }

    if (mprotect(mapping + usable_rounded, page, PROT_NONE) != 0) {
        fprintf(stderr, "mprotect failed: %s\n", strerror(errno));
        exit(1);
    }

    *mapped_size = total;
    return mapping;
}

static int child_dies_with_signal(void (*fn)(void), int expected_signal)
{
    pid_t pid = fork();
    int status;

    CHECK(pid >= 0);
    if (pid == 0) {
        fn();
        _exit(0);
    }

    CHECK(waitpid(pid, &status, 0) == pid);
    return WIFSIGNALED(status) && WTERMSIG(status) == expected_signal;
}

static void write_through_vulnerable_oversized_slice(void)
{
    size_t mapped_size = 0;
    size_t page = page_size();
    unsigned char *mapping = map_guarded(page, &mapped_size);
    Arena arena;
    unsigned char *ptr;

    arena_init(&arena, mapping, SIZE_MAX);
    ptr = arena_vulnerable_alloc(&arena, page + 1U, 1);
    CHECK(ptr != 0);

    ptr[page] = 0xA5U;

    (void)munmap(mapping, mapped_size);
}

static void write_through_hardened_with_false_capacity(void)
{
    size_t mapped_size = 0;
    size_t page = page_size();
    unsigned char *mapping = map_guarded(page, &mapped_size);
    Arena arena;
    unsigned char *ptr = 0;

    arena_init(&arena, mapping, SIZE_MAX);
    CHECK(arena_hardened_alloc(&arena, page + 1U, 1, (void **)&ptr) == ARENA_OK);
    CHECK(ptr != 0);

    ptr[page] = 0xA5U;

    (void)munmap(mapping, mapped_size);
}

static void test_guard_page_catches_false_capacity(void)
{
    CHECK(child_dies_with_signal(write_through_vulnerable_oversized_slice, SIGSEGV));
    CHECK(child_dies_with_signal(write_through_hardened_with_false_capacity, SIGSEGV));
}

static void test_hardened_respects_true_capacity(void)
{
    size_t page = page_size();
    ArenaMapping mapping;
    void *ptr;

    CHECK(arena_mmap_create(&mapping, page, 1) == ARENA_OK);
    CHECK(mapping.arena.capacity == page);

    CHECK(arena_hardened_alloc(&mapping.arena, page + 1U, 1, &ptr) == ARENA_ERR_OOM);
    CHECK(ptr == 0);
    CHECK(mapping.arena.offset == 0U);

    arena_mmap_destroy(&mapping);
    CHECK(mapping.mapping == 0);
}

int main(void)
{
    if (getenv("ARENA_SKIP_INTENTIONAL_SEGV") == 0) {
        test_guard_page_catches_false_capacity();
    }
    test_hardened_respects_true_capacity();
    puts("arena-safety-poc: mmap guard tests passed");
    return 0;
}
