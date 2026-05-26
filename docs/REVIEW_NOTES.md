# C/Systems Review Notes

These notes summarize the local ebook review that informed the second pass of the POC. They are paraphrased working notes, not copied book text.

## Sources Reviewed

- `IT/C Programming Language - 2nd Edition.pdf`
- `IT/C_Interfaces_and_Implementations__Techniques_for_Creating_Reusable_Software.pdf`
- `IT/Programming_Secure_Applications_for_Unix_like_Systems.pdf`
- `IT/Operating Systems - Internals and Design Principles 7th.pdf`
- `IT/Understanding Linux Kernel.pdf`
- `IT/mastering-linux-kernel-development.pdf`
- `IT/GDB_Cookbook.pdf`
- `IT/The.Art.of.Debugging.with.GDB.DDD.and.Eclipse.pdf`
- `IT/optimization_manuals.zip`

## Findings Applied

1. Alignment is not decorative.

   K&R's allocator discussion and the optimization manuals both reinforce that allocators must return storage suitable for the caller's object alignment. The bit-mask formula is valid only for power-of-two alignment. The hardened allocator now makes that a hard contract.

2. Arithmetic checks are necessary but not sufficient.

   The secure Unix material emphasizes validating inputs and avoiding buffer overflow classes. For this allocator, `size`, `alignment`, `offset`, and `capacity` are all inputs to the memory-safety decision, even when they arrive through an internal struct.

3. Virtual memory creates a protection boundary, not a semantic guarantee.

   The kernel and OS books describe virtual address spaces, VMAs, page faults, and guard pages. That maps directly to the POC: if an allocator records a capacity larger than the real mapping, even a checked allocator can return a pointer whose later use crosses into unmapped memory.

4. Guard pages are an excellent diagnostic rail.

   The Linux kernel material discusses guard pages as a way to turn overruns into faults. The POC now uses an mmap-backed guard-page test to prove the failure mode deterministically.

5. Resource lifetime needs one owner.

   C has no native RAII, so the practical pattern is an ownership struct plus explicit cleanup. `ArenaMapping` now stores `mapping`, `mapped_size`, `usable_size`, `page_size`, and the `Arena` together so `munmap` and allocation bounds share one source of truth.

6. Optimization comes after contracts.

   The optimization manuals point toward cache locality, alignment, and compiler-assisted optimization, but this allocator is not bottlenecked by arithmetic checks. The best performance choice here is preserving the simple bump-pointer fast path while making undefined or hostile cases fail before pointer construction.

## Remaining Alarming Facets

- Thread safety: the arena is single-threaded. Shared use needs locking, atomics, or per-thread arenas.
- Zero-size allocation semantics: repeated zero-size allocations can return aliases. The hardened POC now rejects them to keep success meaningful.
- Over-alignment policy: very large alignments can waste capacity quickly. Production APIs may cap alignment to page size or a caller-specified maximum.
- Pointer provenance: integer-to-pointer round trips are useful for testing but deserve caution in production C across non-flat or capability-based architectures.
- False backing capacity: no allocator can prove a caller-owned buffer's true extent from a pointer alone. Owned mapping helpers or caller-side contracts are required.
- Confidential data: arena clear only resets the offset. It does not wipe memory; sensitive arenas need explicit zeroization before reuse or unmap.
- Lifetime confusion: arenas are safe only when users group allocations by shared lifetime. Returning scratch-arena memory as persistent data is a user-contract bug, not something a bump allocator can infer.
- Destructor semantics: raw C arenas do not run destructors or cleanup callbacks. Objects that own file descriptors, heap memory, locks, or other external resources need an explicit release layer above the arena.
- Reallocation semantics: this POC intentionally has no resize API. If one is added, it must track the previous allocation offset and reject stale or out-of-arena pointers.
- Heap exploitation lens: metadata-free arena blocks reduce allocator-metadata corruption opportunities, but an overflow inside one arena allocation can still corrupt adjacent application objects. The allocator cannot replace object-level bounds checks.
