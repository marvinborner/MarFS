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

	FILE *image = fopen(argv[1], "r+");
	if (!image) {
		fprintf(stderr, "%s: error: no valid image specified\n", argv[0]);
		return 1;
	}

	struct marfs_header header = {
		.magic = MARFS_MAGIC,
		.version = MARFS_SPEC_VERSION,
		.entry_size = MARFS_ENTRY_SIZE,
		.main = MARFS_POINT(1, 1),
		.padding = { 0 },
	};

	fseek(image, 0, SEEK_SET);
	size_t write = fwrite(&header, 1, sizeof(header), image);
	if (write != sizeof(header))
		fprintf(stderr, "%s: error: header write failed: %lu bytes written\n", argv[0],
			write);

	struct marfs_dir_entry main = {
		.header.type = MARFS_DIR_ENTRY,
		.header.prev = MARFS_END,
		.header.next = MARFS_END,
	};
	main.entries[0].name[0] = 0;

	write = fwrite(&main, 1, sizeof(main), image);
	if (write != sizeof(main))
		fprintf(stderr, "%s: error: main directory write failed: %lu bytes written\n",
			argv[0], write);

	fflush(image);
	fclose(image);

	printf("MarFS format successful\n");
	return 0;
}
