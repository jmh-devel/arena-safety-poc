# Arena Allocator Safety POC Specification

## Source Claim

The public LinkedIn post by Lumos Lumaday/Vahagn Avagyan describes asking Gemini for a metadata-free linear arena allocator in C. The generated pattern allegedly compiled cleanly but missed integer overflow and alignment hazards. LinkedIn exposed the post body, but not the comment thread without authentication, so this repo is based on the visible claim rather than Joel's hidden comment conversation.

Visible claim summary:

- implement a custom C99/C11 linear arena allocator
- bypass per-object `malloc`/`free`
- slice allocations from one large buffer
- align pointers using bitwise masking such as `~(alignment - 1)`
- store no metadata before blocks
- provide a function to clear the arena without freeing system pointers
- evaluate whether the generated implementation misses overflow and alignment hazards

## Hypothesis Under Test

The unsafe version represents the "AI textbook pattern":

```c
aligned = (current + (alignment - 1)) & ~(alignment - 1);
new_offset = (aligned - base) + size;
if (new_offset > capacity) return NULL;
```

This pattern is only valid if several preconditions are enforced:

- `alignment` is non-zero
- `alignment` is a power of two
- `size` is non-zero so successful calls cannot produce repeated aliasing no-op allocations
- `current + alignment - 1` cannot overflow `uintptr_t`
- `aligned - base` cannot underflow or exceed the arena capacity
- `aligned_offset + size` cannot overflow `size_t`
- hostile or corrupted arena state is rejected instead of laundered into a valid-looking pointer
- the declared capacity actually matches the live backing memory region

The counter-hypothesis is that a metadata-free arena is still viable if these checks become part of the allocator contract.

## Security Contract

A safe allocator must:

1. Allocate from a caller-owned contiguous buffer.
2. Store no per-block metadata before returned pointers.
3. Return pointers aligned to the requested power-of-two alignment.
4. Reject zero and non-power-of-two alignment.
5. Reject zero-size allocations to avoid ambiguous repeated aliases.
6. Reject arithmetic overflow before computing wrapped addresses or offsets.
7. Reject requests that exceed remaining capacity, including requests where alignment padding alone exhausts capacity.
8. Support `clear` by resetting `offset` to zero without freeing the backing buffer.
9. Leave the arena in a coherent state after a rejected request.
10. If the allocator owns an `mmap` region, store the real usable capacity and mapping size together so cleanup and bounds checks use the same source of truth.

## Test Matrix

| Test | Vulnerable Expected | Hardened Expected |
| --- | --- | --- |
| normal allocation | succeeds | succeeds |
| clear/reset | succeeds | succeeds |
| alignment 24 | accepts an unsupported alignment | rejects |
| `uintptr_t` wrap | returns wrapped pointer | rejects |
| `size_t` wrap | returns pointer and small offset | rejects |
| zero-size allocation | returns repeat aliases | rejects |
| alignment padding exhaustion | may be misclassified | rejects as OOM without mutation |
| null or corrupted state | undefined contract | rejects without mutation |
| false mmap capacity | crosses guard page when caller writes returned slice | cannot be solved by arithmetic checks alone |
| owned mmap capacity | n/a | rejects before guard page |
| out of memory | rejects | rejects |

## Implementation Plan

The repo provides:

- `include/arena.h`: common structs and APIs
- `src/arena_vulnerable.c`: intentionally unsafe reference
- `src/arena_hardened.c`: defensive allocator
- `src/arena_mmap.c`: owned mmap helper with optional guard page and explicit cleanup
- `tests/test_arena.c`: deterministic proof tests
- `tests/test_mmap_guard.c`: page-backed guard tests that prove false capacity can cross into unmapped memory
- `fuzz/fuzz_arena.c`: lightweight randomized harness for regression hunting

## Success Criteria

`make test` must show that:

- normal arena behavior works in both versions
- the vulnerable allocator admits the three bad states
- the hardened allocator rejects the same states with explicit error codes
- the guard-page test demonstrates how a false capacity contract can become a real memory fault
- the owned mmap helper keeps mapping lifetime, usable capacity, guard page, and cleanup in one structure

`make sanitize` should pass for the deterministic tests on hosts with working AddressSanitizer/UBSan runtime packages.

## Notes

The tests avoid dereferencing forged pointers. They prove contract violations through returned pointer values, offset mutation, and error codes rather than relying on process crashes.
