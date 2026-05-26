# Arena Safety POC

This repo turns a LinkedIn claim about AI-generated low-level C into a reproducible test:

> A metadata-free arena allocator can look correct, compile cleanly, and still be wrong because unchecked integer arithmetic and bit-mask alignment create hidden memory-safety hazards.

The POC compares two C11 allocators:

- `arena_vulnerable_*`: intentionally uses the common textbook/AI pattern.
- `arena_hardened_*`: uses explicit overflow, bounds, and alignment validation.

The goal is not to ship an allocator. The goal is to make the observation measurable and test Joel's counter-hypothesis: can the pattern be made safe without giving up the useful arena abstraction?

## Quick Start

```sh
make test
make fuzz
make valgrind
make analyze
make sanitize
```

## Current Result

The vulnerable allocator fails the security contract in three deterministic ways:

- accepts non-power-of-two alignments that the bit-mask formula is not defined to satisfy
- permits pointer arithmetic wraparound under hostile state
- permits size addition wraparound, making an oversized request appear to fit
- trusts false arena capacity and can hand back a slice that crosses into an `mmap` guard page

The hardened allocator rejects all three cases while preserving normal arena allocation and clear/reset behavior.

See [docs/SPEC.md](docs/SPEC.md) for the complete specification and [docs/TOOLS.md](docs/TOOLS.md) for the analysis tool matrix.
