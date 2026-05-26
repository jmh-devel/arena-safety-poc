#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "arena.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

static int add_overflows(size_t left, size_t right, size_t *out)
{
    if (left > SIZE_MAX - right) {
        return 1;
    }

    *out = left + right;
    return 0;
}

static int mul_overflows(size_t left, size_t right, size_t *out)
{
    if (left != 0U && right > SIZE_MAX / left) {
        return 1;
    }

    *out = left * right;
    return 0;
}

static ArenaStatus round_up_to_page(size_t value, size_t page, size_t *out)
{
    size_t biased;
    size_t pages;

    if (page == 0U) {
        return ARENA_ERR_SYSTEM;
    }

    if (add_overflows(value, page - 1U, &biased)) {
        return ARENA_ERR_OVERFLOW;
    }

    pages = biased / page;
    if (pages == 0U) {
        pages = 1U;
    }

    if (mul_overflows(pages, page, out)) {
        return ARENA_ERR_OVERFLOW;
    }

    return ARENA_OK;
}

ArenaStatus arena_mmap_create(ArenaMapping *mapping, size_t usable_size, int guard_page)
{
    long page_long;
    size_t page;
    size_t usable_rounded;
    size_t total;
    unsigned char *base;
    ArenaStatus status;

    if (mapping == 0) {
        return ARENA_ERR_NULL;
    }

    memset(mapping, 0, sizeof(*mapping));

    page_long = sysconf(_SC_PAGESIZE);
    if (page_long <= 0) {
        return ARENA_ERR_SYSTEM;
    }

    page = (size_t)page_long;
    status = round_up_to_page(usable_size, page, &usable_rounded);
    if (status != ARENA_OK) {
        return status;
    }

    total = usable_rounded;
    if (guard_page && add_overflows(total, page, &total)) {
        return ARENA_ERR_OVERFLOW;
    }

    base = mmap(0, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        return ARENA_ERR_SYSTEM;
    }

    if (guard_page && mprotect(base + usable_rounded, page, PROT_NONE) != 0) {
        (void)munmap(base, total);
        return ARENA_ERR_SYSTEM;
    }

    mapping->mapping = base;
    mapping->mapped_size = total;
    mapping->usable_size = usable_size;
    mapping->page_size = page;
    arena_init(&mapping->arena, base, usable_size);
    return ARENA_OK;
}

void arena_mmap_destroy(ArenaMapping *mapping)
{
    if (mapping == 0 || mapping->mapping == 0 || mapping->mapped_size == 0U) {
        return;
    }

    (void)munmap(mapping->mapping, mapping->mapped_size);
    memset(mapping, 0, sizeof(*mapping));
}
