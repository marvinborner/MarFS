# Copyright (c) 2021, Marvin Borner <melvix@marvinborner.de>
# SPDX-License-Identifier: MIT

CC = gcc
LD = ld
SU = sudo
TG = ctags

BUILD = $(PWD)/build
SRC = ./src
INC = ./inc
SRCS = $(shell find $(SRC) -type f -name '*.c')
OBJS = $(SRCS:%=$(BUILD)/%.o)

CFLAGS_WARNINGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wnested-externs -Wformat=2 -Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes -Wcast-qual -Wswitch-default -Wswitch-enum -Wunreachable-code -Wundef -Wold-style-definition -Wvla -pedantic-errors
CFLAGS = $(CFLAGS_WARNINGS) -std=c99 -fno-profile-generate -fno-omit-frame-pointer -fno-common -fno-asynchronous-unwind-tables -mno-red-zone -Ofast -I$(INC)

all: $(BUILD) $(OBJS)

compile: all sync

clean:
	@rm -rf $(BUILD)/*

clean_disk:
	@rm -rf $(BUILD)/*.img

run: compile $(BUILD) clean_disk $(BUILD)/disk.img

sync: # Ugly hack
	@$(MAKE) --always-make --dry-run | grep -wE 'gcc' | jq -nR '[inputs|{directory: ".", command: ., file: match(" [^ ]+$$").string[1:]}]' >compile_commands.json &
	@$(TG) -R --exclude=.git --exclude=build . &

$(BUILD)/%.c.o: %.c
	@$(CC) $(CFLAGS) $< -o $(patsubst $(BUILD)/$(SRC)/%.c.o,$(BUILD)/marfs_%,$@)

.PHONY: all run compile clean clean_disk sync

$(BUILD):
	@mkdir -p $@

%.img: $(BUILD)
	@dd if=/dev/zero of=$@ bs=1k count=32k status=none
	@DEV=$$($(SU) losetup --find --partscan --show $@) && \
	PART="p1" && \
	$(SU) parted -s "$$DEV" mklabel msdos mkpart primary 32k 100% -a minimal set 1 boot on && \
	$(SU) ./build/marfs_mkfs "$$DEV$$PART" && \
	$(SU) losetup -d "$$DEV"
