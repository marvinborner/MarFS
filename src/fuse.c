/**
 * Copyright (c) 2021, Marvin Borner <melvix@marvinborner.de>
 * SPDX-License-Identifier: MIT
 */

#define FUSE_USE_VERSION 30

#include <spec.h>

#include <errno.h>
#include <fuse.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static char *image = 0;

static void marfs_debug(const char *fmt, ...)
{
	printf("\t>>> ");
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	printf("\n");
}

static void *marfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	marfs_debug("init()");
	UNUSED(conn);
	cfg->kernel_cache = 1;
	return NULL;
}

static void marfs_destroy(void *data)
{
	marfs_debug("destroy()");
	if (image)
		free(image);
}

static int marfs_open(const char *file_path, struct fuse_file_info *file_info)
{
	marfs_debug("open() on %s", file_path);
	return 0;
}

static int marfs_opendir(const char *dir_path, struct fuse_file_info *file_info)
{
	marfs_debug("opendir() on %s", dir_path);
	return 0;
}

static int marfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *info)
{
	marfs_debug("getattr() on %s", path);
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
	marfs_debug("readdir() on %s and offset %lu", path, offset);

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
	marfs_debug("release() on %s", path);
	return 0;
}

static int marfs_releasedir(const char *path, struct fuse_file_info *file_info)
{
	marfs_debug("releasedir() on %s", path);
	return 0;
}

static int marfs_read(const char *path, char *buf, size_t to_read, off_t offset,
		      struct fuse_file_info *file_info)
{
	marfs_debug("read() on %s, %lu", path, to_read);
	return to_read;
}

static int marfs_write(const char *path, const char *buf, size_t to_write, off_t offset,
		       struct fuse_file_info *file_info)
{
	marfs_debug("write() on %s", path);
	return to_write;
}

static int marfs_create(const char *path, mode_t mode, struct fuse_file_info *file_info)
{
	marfs_debug("create() on %s", path);
	return 0;
}

static int marfs_mkdir(const char *path, mode_t mode)
{
	marfs_debug("mkdir() on %s", path);
	return 0;
}

static int marfs_unlink(const char *path)
{
	marfs_debug("unlink() on %s", path);
	return 0;
}

static int marfs_rmdir(const char *path)
{
	marfs_debug("rmdir() on %s", path);
	return 0;
}

static int marfs_utimens(const char *path, const struct timespec tv[2],
			 struct fuse_file_info *file_info)
{
	marfs_debug("utimens() on %s", path);
	return 0;
}

static int marfs_truncate(const char *path, off_t size, struct fuse_file_info *file_info)
{
	marfs_debug("truncate() on %s, size %lu", path, size);
	return 0;
}

static int marfs_rename(const char *path, const char *new, unsigned int flags)
{
	marfs_debug("rename() on %s -> %s", path, new);
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
	image = strdup(argv[1]);
	argv++;
	argc--;

	fuse_main(argc, argv, &operations, NULL);

	return 0;
}
