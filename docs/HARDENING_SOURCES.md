# Hardening Sources

This file records the source trail behind the hardened allocator contract. It is intentionally explicit so future changes can be judged against the same assumptions.

## External Articles

| Source | Determination Impact |
| --- | --- |
| Ginger Bill, "Memory Allocation Strategies - Part 2" - https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/ | Power-of-two alignment is a required precondition for bit-mask alignment. Reset/free-all, temporary savepoints, and potential resize APIs must be treated as explicit contract surfaces. |
| Ryan Fleury, "Untangling Lifetimes: The Arena Allocator" - https://www.dgtlgrove.com/p/untangling-lifetimes-the-arena-allocator | Arena safety depends on lifetime grouping. Scratch arenas need conflict handling so temporary storage is not returned as persistent data. |
| Ng Song Guan, "High Performance Memory Management: Arena Allocators" - https://medium.com/@sgn00/high-performance-memory-management-arena-allocators-c685c81ee338 | Fixed-capacity arenas should fail clearly unless the design explicitly adds chained growth or fallback allocation. Growth/fallback add ownership and cleanup complexity. |
| Mouad Kondah, "Breaking Memory Safety in the Heap Arena" - https://www.deep-kondah.com/breaking-memory-safety-in-the-heap-arena/ | Avoiding per-block allocator metadata reduces metadata-corruption exploit surface, but intra-arena object overflows can still corrupt application state. Sanitizers, guard pages, and telemetry are complementary, not substitutes for bounds-safe usage. |

## Local Book/Reference Review

| Source Group | Determination Impact |
| --- | --- |
| `IT/C Programming Language - 2nd Edition.pdf` | Allocator alignment and pointer arithmetic need strict, simple contracts. |
| `IT/C_Interfaces_and_Implementations__Techniques_for_Creating_Reusable_Software.pdf` | The public API must expose the real invariants instead of hiding contract-breaking behavior behind convenience. |
| `IT/Programming_Secure_Applications_for_Unix_like_Systems.pdf` | Treat allocator fields and caller inputs as untrusted at the boundary; reject overflow, invalid alignment, and corrupted state before pointer construction. |
| `IT/Operating Systems - Internals and Design Principles 7th.pdf`, `IT/Understanding Linux Kernel.pdf`, `IT/mastering-linux-kernel-development.pdf` | Virtual memory, page faults, and guard pages are diagnostic/protection rails, but a pointer/capacity pair alone cannot prove the true backing extent. |
| `IT/GDB_Cookbook.pdf`, `IT/The.Art.of.Debugging.with.GDB.DDD.and.Eclipse.pdf` | Keep failures reproducible and inspectable through deterministic tests before relying on dynamic tools. |
| `IT/optimization_manuals.zip` | Alignment and locality matter, but optimization should not weaken the safety contract. |

## Contract Decisions

- Reject zero and non-power-of-two alignment.
- Reject zero-size allocations so success cannot mean "returned a repeated no-op alias."
- Reject `uintptr_t` and `size_t` overflow before deriving returned pointers or offsets.
- Reject corrupted state, including `offset > capacity`.
- Treat padding-only capacity exhaustion as out-of-memory without mutating state.
- Keep rejected allocations from mutating `offset` or returning stale output pointers.
- Keep user-provided backing memory and owned `mmap` mappings as separate trust levels.
- Avoid per-block metadata in the POC; adding resize, free, growth, fallback allocation, or destructors must come with new metadata, ownership rules, and tests.
- Document that arenas do not prevent use-after-reset, scratch lifetime confusion, concurrent races, object-level overflows, or missing destructor cleanup.
