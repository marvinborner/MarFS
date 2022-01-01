/*
 * Copyright (c) 2021, Marvin Borner <melvix@marvinborner.de>
 * SPDX-License-Identifier: MIT
 */

#ifndef MARFS_SPEC
#define MARFS_SPEC

#include <def.h>

#define MARFS_POINT(head, tail) ((struct marfs_pointer){ (head), (tail) })
#define MARFS_MAGIC "MarFS"
#define MARFS_SPEC_VERSION 1

#define MARFS_END 0xffffffff
#define MARFS_NAME_LENGTH 32
#define MARFS_ENTRY_SIZE 1024

#define MARFS_DIR_ENTRY 1
#define MARFS_FILE_ENTRY 2

#define MARFS_DIR_ENTRY_COUNT (MARFS_ENTRY_SIZE / sizeof(struct marfs_dir_entry_data))

struct marfs_pointer {
	u32 head, tail;
} PACKED;

struct marfs_header {
	char magic[6];
	u32 version;
	u32 entry_size;
	struct marfs_pointer main;
	u8 padding[MARFS_ENTRY_SIZE - 22];
} PACKED;

struct marfs_entry_header {
	u32 type;
	u32 id;
	u32 prev, next; // MARFS_END if end
} PACKED;

struct marfs_dir_entry_data {
	char name[MARFS_NAME_LENGTH + 1];
	u32 length;
	struct marfs_pointer pointer;
} PACKED;

struct marfs_dir_entry {
	struct marfs_entry_header header;
	u32 count;
	struct marfs_dir_entry_data entries[MARFS_DIR_ENTRY_COUNT];
	u8 padding[MARFS_ENTRY_SIZE - sizeof(struct marfs_entry_header) - sizeof(u32) -
		   (MARFS_DIR_ENTRY_COUNT * sizeof(struct marfs_dir_entry_data))];
} PACKED;

struct marfs_file_entry {
	struct marfs_entry_header header;
	u32 size;
	u8 data[MARFS_ENTRY_SIZE - sizeof(struct marfs_entry_header) - sizeof(u32)];
} PACKED;

#endif
