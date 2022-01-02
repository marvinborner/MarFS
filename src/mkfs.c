/**
 * Copyright (c) 2021, Marvin Borner <melvix@marvinborner.de>
 * SPDX-License-Identifier: MIT
 */

#include <spec.h>

#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	// Verify that spec is correct
	assert(sizeof(struct marfs_header) == MARFS_ENTRY_SIZE);
	assert(sizeof(struct marfs_dir_entry) == MARFS_ENTRY_SIZE);
	assert(sizeof(struct marfs_file_entry) == MARFS_ENTRY_SIZE);

	if (argc < 2) {
		fprintf(stderr, "%s: error: no image specified\n", argv[0]);
		return 1;
	}

	printf("Formatting %s...\n", argv[1]);

	FILE *image = fopen(argv[1], "r+");
	if (!image) {
		fprintf(stderr, "%s: error: no valid image specified\n", argv[0]);
		return 1;
	}

	fseek(image, 0, SEEK_END);
	size_t size = ftell(image);

	if (size < 2 * MARFS_ENTRY_SIZE) {
		fprintf(stderr, "%s: error: too small image size: %luB but at least %dB needed\n",
			argv[0], size, 2 * MARFS_ENTRY_SIZE);
		return 1;
	}

	struct marfs_entry_header entry_header = { 0 };
	for (u32 i = 0; i < size / MARFS_ENTRY_SIZE; i++) {
		fseek(image, i * MARFS_ENTRY_SIZE, SEEK_SET);
		assert(fwrite(&entry_header, 1, sizeof(entry_header), image) ==
		       sizeof(entry_header));
	}

	struct marfs_header header = {
		.magic = MARFS_MAGIC,
		.version = MARFS_SPEC_VERSION,
		.entry_size = MARFS_ENTRY_SIZE,
		.entry_count = size / MARFS_ENTRY_SIZE,
		.main = MARFS_POINT(1, 1),
		.padding = { 0 },
	};

	fseek(image, 0, SEEK_SET);
	size_t write = fwrite(&header, 1, sizeof(header), image);
	if (write != sizeof(header))
		fprintf(stderr, "%s: error: header write failed: %lu bytes written\n", argv[0],
			write);

	struct marfs_dir_entry_info info = {
		.header.type = MARFS_DIR_ENTRY,
		.header.id = 1,
		.header.prev = MARFS_END,
		.header.next = MARFS_END,
		.count = 0,
	};

	write = fwrite(&info, 1, sizeof(info), image);
	if (write != sizeof(info))
		fprintf(stderr, "%s: error: main directory write failed: %lu bytes written\n",
			argv[0], write);

	fflush(image);
	fclose(image);

	printf("MarFS format successful\n");
	return 0;
}
