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
DEMO_BIN := $(BUILD)/arena_demo
MMAP_TEST_BIN := $(BUILD)/test_mmap_guard

SRC := src/arena_vulnerable.c src/arena_hardened.c src/arena_mmap.c
OBJ := $(SRC:src/%.c=$(BUILD)/%.o)

.PHONY: all test sanitize fuzz demo valgrind analyze clean spec

all: $(LIB) $(DEMO_BIN)

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
	@mkdir -p $(BUILD); \
	if printf 'int main(void){return 0;}\n' | $(CC) -x c -fsanitize=address,undefined -o $(BUILD)/sanitize-probe - >/dev/null 2>&1; then \
		rm -f $(BUILD)/sanitize-probe; \
		$(MAKE) clean; \
		ARENA_SKIP_INTENTIONAL_SEGV=1 $(MAKE) test CFLAGS="$(CSTD) $(WARN) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address,undefined"; \
		status=$$?; \
		$(MAKE) clean; \
		exit $$status; \
	else \
		echo "skip: sanitizer runtime is not available for $(CC)"; \
	fi

$(FUZZ_BIN): fuzz/fuzz_arena.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) -o $@

fuzz: $(FUZZ_BIN)
	$(FUZZ_BIN) 100000

$(DEMO_BIN): examples/arena_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) -o $@

demo: $(DEMO_BIN)
	$(DEMO_BIN)

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
