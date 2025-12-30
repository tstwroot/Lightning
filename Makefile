APP_NAME := test_lightning
CC := gcc
SRC_DIR := src
INC_DIR := include

CFLAGS_COMMON := -std=c17 -Wall -Wextra -Wshadow -I$(INC_DIR) -MMD -MP

CFLAGS_DEBUG   := $(CFLAGS_COMMON) -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
CFLAGS_RELEASE := $(CFLAGS_COMMON) -O3 -march=native -mtune=native -flto -DNDEBUG \
                  -fomit-frame-pointer -ffast-math -funroll-loops \
                  -finline-functions -fprefetch-loop-arrays

SRCS := $(APP_NAME).c $(wildcard $(SRC_DIR)/core/*.c) $(wildcard $(SRC_DIR)/http/*.c)
OBJS_DEBUG   := $(SRCS:%.c=build/debug/%.o)
OBJS_RELEASE := $(SRCS:%.c=build/release/%.o)

.PHONY: all debug release clean

all: debug

debug: CFLAGS := $(CFLAGS_DEBUG)
debug: LDFLAGS := -fsanitize=address,undefined
debug: $(APP_NAME)_debug

$(APP_NAME)_debug: $(OBJS_DEBUG)
	$(CC) $(OBJS_DEBUG) -o $@ $(LDFLAGS)
	@echo "Build DEBUG criado: ./$@"

release: CFLAGS := $(CFLAGS_RELEASE)
release: LDFLAGS := -flto
release: $(APP_NAME)

$(APP_NAME): $(OBJS_RELEASE)
	$(CC) $(OBJS_RELEASE) -o $@ $(LDFLAGS)
	@echo "Build RELEASE criado: ./$@"

build/debug/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/release/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(APP_NAME) $(APP_NAME)_debug
	find . -name "*.d" -delete

-include build/*/*.d
