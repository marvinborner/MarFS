/**
 * Copyright (c) 2021, Marvin Borner <melvix@marvinborner.de>
 * SPDX-License-Identifier: MIT
 */

#define FUSE_USE_VERSION 30

#include <spec.h>

#include <assert.h>
#include <errno.h>
#include <fuse.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static FILE *image = NULL;

static struct marfs_header header = { 0 };

static void debug(const char *fmt, ...)
{
	printf("\t>>> ");
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	printf("\n");
}

static void sector(u32 index, u32 size, void *buf)
{
	fseek(image, index * header.entry_size, SEEK_SET);
	assert(fread(buf, 1, size, image) == size);
}

// first -> start = header.main.head
static u8 read_entry_header(const char *path, u32 start, struct marfs_entry_header *entry_header)
{
	while (*path && *path == '/')
		path++;

	struct marfs_entry_header h;
	sector(start, sizeof(h), &h);

	if (h.type == MARFS_DIR_ENTRY) {
		struct marfs_dir_entry entry;
		sector(start, sizeof(entry), &entry);
		assert(entry.count <= MARFS_DIR_ENTRY_COUNT);
		for (u32 i = 0; i < entry.count; i++) {
			struct marfs_dir_entry_data *data = &entry.entries[i];
			if (!strncmp(path, data->name, data->length)) {
				if (path[data->length] == '/') {
					return read_entry_header(path, data->pointer.head,
								 entry_header);
				}
				if (!path[data->length]) {
					*entry_header = h;
					return 1;
				}
			}
		}

		if (entry.header.next != MARFS_END)
			return read_entry_header(path, entry.header.next, entry_header);
	} else if (h.type == MARFS_FILE_ENTRY) {
		*entry_header = h;
		return 1;
	}

	return 0;
}

/**
 * Fuse operations
 */

static void *marfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	debug("init()");
	UNUSED(conn);
	cfg->kernel_cache = 1;
	return NULL;
}

static void marfs_destroy(void *data)
{
	debug("destroy()");
	UNUSED(data);

	if (image)
		fclose(image);
}

static int marfs_open(const char *file_path, struct fuse_file_info *file_info)
{
	debug("open() on %s", file_path);
	return 0;
}

static int marfs_opendir(const char *dir_path, struct fuse_file_info *file_info)
{
	debug("opendir() on %s", dir_path);
	return 0;
}

static int marfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *info)
{
	debug("getattr() on %s", path);
	UNUSED(info);

	memset(stat, 0, sizeof(*stat));
	if (strcmp(path, "/") == 0) {
		stat->st_mode = S_IFDIR | 0000;
	} else if (strcmp(path + 1, "aah") == 0) {
		stat->st_mode = S_IFREG | 0000;
		stat->st_size = 0;
	} else {
		return -ENOENT;
	}

	return 0;
}

static int marfs_readdir(const char *path, void *buf, fuse_fill_dir_t fill, off_t offset,
			 struct fuse_file_info *file_info, enum fuse_readdir_flags flags)
{
	debug("readdir() on %s and offset %lu", path, offset);

	UNUSED(offset);
	UNUSED(file_info);
	UNUSED(flags);
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	fill(buf, ".", NULL, 0, 0);
	fill(buf, "..", NULL, 0, 0);
	fill(buf, "aah", NULL, 0, 0);

	return 0;
}

static int marfs_release(const char *path, struct fuse_file_info *file_info)
{
	debug("release() on %s", path);
	return 0;
}

static int marfs_releasedir(const char *path, struct fuse_file_info *file_info)
{
	debug("releasedir() on %s", path);
	return 0;
}

static int marfs_read(const char *path, char *buf, size_t to_read, off_t offset,
		      struct fuse_file_info *file_info)
{
	debug("read() on %s, %lu", path, to_read);
	return to_read;
}

static int marfs_write(const char *path, const char *buf, size_t to_write, off_t offset,
		       struct fuse_file_info *file_info)
{
	debug("write() on %s", path);
	return to_write;
}

static int marfs_create(const char *path, mode_t mode, struct fuse_file_info *file_info)
{
	debug("create() on %s", path);
	return 0;
}

static int marfs_mkdir(const char *path, mode_t mode)
{
	debug("mkdir() on %s", path);
	return 0;
}

static int marfs_unlink(const char *path)
{
	debug("unlink() on %s", path);
	return 0;
}

static int marfs_rmdir(const char *path)
{
	debug("rmdir() on %s", path);
	return 0;
}

static int marfs_utimens(const char *path, const struct timespec tv[2],
			 struct fuse_file_info *file_info)
{
	debug("utimens() on %s", path);
	return 0;
}

static int marfs_truncate(const char *path, off_t size, struct fuse_file_info *file_info)
{
	debug("truncate() on %s, size %lu", path, size);
	return 0;
}

static int marfs_rename(const char *path, const char *new, unsigned int flags)
{
	debug("rename() on %s -> %s", path, new);
	return 0;
}

static struct fuse_operations operations = {
	.init = marfs_init,
	.destroy = marfs_destroy,
	.open = marfs_open,
	.opendir = marfs_opendir,
	.getattr = marfs_getattr,
	.readdir = marfs_readdir,
	.release = marfs_release,
	.releasedir = marfs_releasedir,
	.read = marfs_read,
	.write = marfs_write,
	.create = marfs_create,
	.unlink = marfs_unlink,
	.utimens = marfs_utimens,
	.truncate = marfs_truncate,
	.mkdir = marfs_mkdir,
	.rmdir = marfs_rmdir,
	.rename = marfs_rename,
};

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "error: no image specified\n");
		exit(1);
	}

	// I don't think that's how fuse is supposed to work but idc
	char *path = argv[1];
	argv++;
	argc--;

	image = fopen(path, "r");
	if (!image) {
		fprintf(stderr, "error: invalid image path '%s' specified: %s\n", path,
			strerror(errno));
		exit(1);
	}

	int head_read = fread(&header, 1, sizeof(header), image);
	if (head_read != sizeof(header)) {
		fprintf(stderr, "error: could not read full header: %d\n", head_read);
		fclose(image);
		exit(1);
	}

	if (strcmp(header.magic, MARFS_MAGIC) != 0) {
		fprintf(stderr, "error: invalid header magic: %s\n", header.magic);
		fclose(image);
		exit(1);
	}

	if (header.version != MARFS_SPEC_VERSION) {
		fprintf(stderr, "error: non-supported spec-version %d\n", header.version);
		fclose(image);
		exit(1);
	}

	fuse_main(argc, argv, &operations, NULL);

	return 0;
}
