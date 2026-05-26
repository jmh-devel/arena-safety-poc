# Analysis Tool Matrix

This POC is intentionally small enough to inspect by hand, but allocator bugs deserve multiple lenses.

## Included Targets

| Target | Purpose |
| --- | --- |
| `make test` | Deterministic contract tests for vulnerable versus hardened allocator behavior. |
| `make fuzz` | Lightweight randomized checks against the hardened allocator contract. |
| `make valgrind` | Runs deterministic tests under Memcheck when Valgrind is installed. |
| `make analyze` | Runs GCC's `-fanalyzer` static analyzer when supported by the host compiler; otherwise reports a skip. |
| `make sanitize` | Runs AddressSanitizer/UBSan when the host has working sanitizer runtimes; otherwise reports a skip. |

## Extra Tools Worth Running

| Tool | Use |
| --- | --- |
| AddressSanitizer | Detects real out-of-bounds and use-after-free behavior during execution. |
| UndefinedBehaviorSanitizer | Detects undefined C behavior that may not crash. |
| Valgrind Memcheck | Slower runtime memory checker, useful when sanitizer runtimes are unavailable. |
| GCC `-fanalyzer` | Compiler-integrated static path analysis. |
| Clang Static Analyzer / `scan-build` | Independent static path analysis. |
| `cppcheck` | Lightweight static checks for portability and defensive C mistakes. |
| `clang-tidy` | Style and bug-prone pattern checks when a compile database exists. |
| AFL++ / libFuzzer | Coverage-guided fuzzing if the allocator API expands. |
| Guard pages | Deterministically catches writes that cross a protected page boundary. |
| Hardware memory tagging / HWASan | Catches some spatial and temporal defects with different tradeoffs than ASan; useful when available, but not deterministic enough to be the only gate. |

## Alarming Behaviors To Watch

- Non-power-of-two alignment accepted by a power-of-two-only bit-mask formula.
- `uintptr_t` addition wrapping before alignment.
- `size_t` addition wrapping after alignment.
- Arena state corruption where `offset > capacity`.
- Caller or attacker-controlled false capacity over an `mmap` region.
- Returned slices crossing into guard pages or unmapped pages.
- Intra-arena overflows that corrupt adjacent application objects without touching allocator metadata.
- Zero-size allocation semantics that accidentally create aliasing assumptions.
- Over-aligned requests larger than the backing page or system-supported alignment.
- Pointer provenance assumptions hidden by integer-to-pointer round trips.

`make valgrind` sets `ARENA_SKIP_INTENTIONAL_SEGV=1` for the mmap guard binary so the expected child-process guard-page crash does not make Memcheck noisy. Plain `make test` still runs the fault-path proof.
