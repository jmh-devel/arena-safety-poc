# External Arena Review - 2026-05-26

This review cross-checks the POC against external allocator discussions that affected the hardening contract.

## External Sources

- Ginger Bill, "Memory Allocation Strategies - Part 2", https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
  - Impact: linear arena basics, power-of-two alignment, `prev_offset` for resize, free-all/reset, and temporary savepoints.
- Ryan Fleury, "Untangling Lifetimes: The Arena Allocator", https://www.dgtlgrove.com/p/untangling-lifetimes-the-arena-allocator
  - Impact: arenas are primarily a lifetime-design tool; scratch arenas need conflict handling so temporary allocations are not accidentally returned as persistent data.
- Ng Song Guan, "High Performance Memory Management: Arena Allocators", https://medium.com/@sgn00/high-performance-memory-management-arena-allocators-c685c81ee338
  - Impact: fixed-capacity arenas should either fail, grow by chained blocks, or fall back to `malloc`, but growing/fallback behavior adds management complexity.
- Mouad Kondah, "Breaking Memory Safety in the Heap Arena", https://www.deep-kondah.com/breaking-memory-safety-in-the-heap-arena/
  - Impact: heap exploitability often comes from allocator metadata corruption, but application-object corruption remains dangerous even when allocator metadata is absent. This reinforces the POC's no-per-block-metadata design while clarifying that guard pages and arithmetic checks do not prevent every intra-arena overflow.

## Impact On This POC

The arithmetic hardening remains valid: non-power-of-two alignment, integer overflow, corrupted offset, and false capacity are still the core memory-safety hazards in this metadata-free allocator shape.

The external review adds a broader contract lens:

- A successful allocation should mean a real non-empty slice was reserved. Zero-size success creates repeated aliases and unclear ownership, so the hardened allocator now rejects `size == 0`.
- Alignment padding can exhaust capacity before payload size is considered. That is an out-of-memory condition, not arena-state corruption.
- Rejected allocations must not mutate `offset` or leave stale output pointers behind.
- Lifetime misuse is outside what the allocator can infer. Scratch arenas, temporary marks, and caller-owned persistent arenas require API-level rules.
- This POC intentionally avoids `resize`, chained growth, and fallback-to-`malloc` because each one changes the ownership contract and needs separate tests.
- Metadata-free blocks remove an entire class of allocator-metadata overwrite attacks, but they do not protect adjacent application objects inside the same arena.
- Runtime tooling matters: ASan/UBSan, Valgrind, static analysis, and guard pages catch different parts of the spatial/temporal memory-safety problem.

## Remaining Out-Of-Scope Failure Modes

- Concurrent mutation of one arena by multiple threads.
- Returning scratch-arena memory to a longer-lived caller.
- Resetting or destroying an arena while live pointers are still in use.
- Storing objects that require destructors or cleanup callbacks.
- Trusting a caller-provided pointer/capacity pair when the true backing extent is smaller.
- Expanding into multiple blocks without recording block ownership and cleanup order.
- Adding resize without tracking previous allocation metadata or rejecting stale pointers.
- Intra-arena object overflow that corrupts a later object's fields or function pointers without touching allocator metadata.
- Treating this arena as a security boundary. It is a safer allocation primitive, not a substitute for bounds-checked writes, parser validation, or object-level invariants.

These are not invisible bugs in the current hardened allocator; they are missing higher-level APIs and usage contracts. They should stay explicit so future "helpful" feature additions do not quietly recreate a general heap allocator with weaker invariants.
