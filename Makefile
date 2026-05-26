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

SRC := src/arena_vulnerable.c src/arena_hardened.c
OBJ := $(SRC:src/%.c=$(BUILD)/%.o)

.PHONY: all test sanitize fuzz clean spec

all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c include/arena.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(TEST_BIN): tests/test_arena.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) -o $@

test: $(TEST_BIN)
	$(TEST_BIN)

sanitize:
	$(MAKE) clean
	$(MAKE) test CFLAGS="$(CSTD) $(WARN) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address,undefined"

$(FUZZ_BIN): fuzz/fuzz_arena.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB) $(LDFLAGS) -o $@

fuzz: $(FUZZ_BIN)
	$(FUZZ_BIN) 100000

spec:
	sed -n '1,240p' docs/SPEC.md

clean:
	rm -rf $(BUILD)

