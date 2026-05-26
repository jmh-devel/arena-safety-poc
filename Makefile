CC ?= cc
AR ?= ar

CSTD ?= -std=c11
WARN ?= -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes
OPT ?= -O2
CPPFLAGS ?= -Iinclude
CFLAGS ?= $(CSTD) $(WARN) $(OPT) -g
LDFLAGS ?=

BUILD := build
LIB := $(BUILD)/libarena_poc.a
TEST_BIN := $(BUILD)/test_arena
FUZZ_BIN := $(BUILD)/fuzz_arena
MMAP_TEST_BIN := $(BUILD)/test_mmap_guard

SRC := src/arena_vulnerable.c src/arena_hardened.c src/arena_mmap.c
OBJ := $(SRC:src/%.c=$(BUILD)/%.o)

.PHONY: all test sanitize fuzz valgrind analyze clean spec

all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c include/arena.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(TEST_BIN): tests/test_arena.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) -o $@

$(MMAP_TEST_BIN): tests/test_mmap_guard.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) -o $@

test: $(TEST_BIN) $(MMAP_TEST_BIN)
	$(TEST_BIN)
	$(MMAP_TEST_BIN)

sanitize:
	@if printf 'int main(void){return 0;}\n' | $(CC) -x c -fsanitize=address,undefined -o /tmp/arena-sanitize-probe - >/dev/null 2>&1; then \
		$(MAKE) clean; \
		$(MAKE) test CFLAGS="$(CSTD) $(WARN) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address,undefined"; \
	else \
		echo "skip: sanitizer runtime is not available for $(CC)"; \
	fi

$(FUZZ_BIN): fuzz/fuzz_arena.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) -o $@

fuzz: $(FUZZ_BIN)
	$(FUZZ_BIN) 100000

valgrind: $(TEST_BIN) $(MMAP_TEST_BIN)
	valgrind --quiet --error-exitcode=1 --leak-check=full $(TEST_BIN)
	ARENA_SKIP_INTENTIONAL_SEGV=1 valgrind --quiet --error-exitcode=1 --leak-check=full $(MMAP_TEST_BIN)

analyze:
	@if $(CC) -fanalyzer -x c -fsyntax-only /dev/null >/dev/null 2>&1; then \
		$(CC) $(CPPFLAGS) $(CSTD) $(WARN) -fanalyzer -fsyntax-only $(SRC) tests/test_arena.c tests/test_mmap_guard.c fuzz/fuzz_arena.c; \
	else \
		echo "skip: $(CC) does not support -fanalyzer"; \
	fi

spec:
	sed -n '1,240p' docs/SPEC.md

clean:
	rm -rf $(BUILD)
