#include "arena.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    ArenaMapping mapping;
    char *message = 0;
    char *scratch = 0;
    ArenaStatus status;

    status = arena_mmap_create(&mapping, 4096U, 1);
    if (status != ARENA_OK) {
        fprintf(stderr, "arena_mmap_create failed: %s\n", arena_status_name(status));
        return 1;
    }

    status = arena_hardened_alloc(&mapping.arena, 64U, 16U, (void **)&message);
    if (status != ARENA_OK) {
        fprintf(stderr, "message allocation failed: %s\n", arena_status_name(status));
        arena_mmap_destroy(&mapping);
        return 1;
    }

    status = arena_hardened_alloc(&mapping.arena, 128U, 32U, (void **)&scratch);
    if (status != ARENA_OK) {
        fprintf(stderr, "scratch allocation failed: %s\n", arena_status_name(status));
        arena_mmap_destroy(&mapping);
        return 1;
    }

    (void)snprintf(message, 64U, "arena demo ok");
    (void)memset(scratch, 0xA5, 128U);

    printf("%s: capacity=%zu offset=%zu\n",
           message, mapping.arena.capacity, mapping.arena.offset);

    arena_mmap_destroy(&mapping);
    return 0;
}
