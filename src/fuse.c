/**
 * Copyright (c) 2021, Marvin Borner <melvix@marvinborner.de>
 * SPDX-License-Identifier: MIT
 */

#define FUSE_USE_VERSION 30

#include <spec.h>

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
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}

static void *marfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	UNUSED(conn);
	cfg->kernel_cache = 1;
	return NULL;
}

static void marfs_destroy(void *data)
{
	if (image)
		free(image);
}

static int marfs_open(const char *file_path, struct fuse_file_info *file_info)
{
	marfs_debug("opening file %s\n", file_path);
	return 0;
}

static int marfs_opendir(const char *dir_path, struct fuse_file_info *file_info)
{
	marfs_debug("opening dir %s\n", dir_path);
	return 0;
}

static int marfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *info)
{
	marfs_debug("getattr() on %s\n", path);
	return 0;
}

static int marfs_readdir(const char *path, void *buf, fuse_fill_dir_t fill, off_t offset,
			 struct fuse_file_info *file_info, enum fuse_readdir_flags flags)
{
	marfs_debug("readdir() on %s and offset %lu\n", path, offset);
	return 0;
}

static int marfs_release(const char *path, struct fuse_file_info *file_info)
{
	return 0;
}

static int marfs_releasedir(const char *path, struct fuse_file_info *file_info)
{
	return 0;
}

static int marfs_read(const char *path, char *buf, size_t to_read, off_t offset,
		      struct fuse_file_info *file_info)
{
	marfs_debug("marfs_read() on %s, %lu\n", path, to_read);
	return to_read;
}

static int marfs_write(const char *path, const char *buf, size_t to_write, off_t offset,
		       struct fuse_file_info *file_info)
{
	marfs_debug("marfs_write() on %s\n", path);
	return to_write;
}

static int marfs_create(const char *path, mode_t mode, struct fuse_file_info *file_info)
{
	marfs_debug("marfs_create() on %s\n", path);
	return 0;
}

static int marfs_mkdir(const char *path, mode_t mode)
{
	marfs_debug("marfs_mkdir() on %s\n", path);
	return 0;
}

static int marfs_unlink(const char *path)
{
	return 0;
}

static int marfs_rmdir(const char *path)
{
	return 0;
}

static int marfs_utimens(const char *path, const struct timespec tv[2],
			 struct fuse_file_info *file_info)
{
	marfs_debug("marfs_utimens() on %s\n", path);
	return 0;
}

static int marfs_truncate(const char *path, off_t size, struct fuse_file_info *file_info)
{
	marfs_debug("marfs_truncate() on %s, size %lu\n", path, size);
	return 0;
}

static int marfs_rename(const char *path, const char *new, unsigned int flags)
{
	marfs_debug("marfs_rename() on %s, %s\n", path, new);
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
