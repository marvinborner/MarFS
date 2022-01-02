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

static void read_sector(u32 index, u32 size, void *buf)
{
	assert(index < header.entry_count);
	assert(fseek(image, index * header.entry_size, SEEK_SET) == 0);
	assert(fread(buf, 1, size, image) == size);
}

static void write_sector(u32 index, u32 size, void *buf)
{
	assert(index < header.entry_count);
	assert(fseek(image, index * header.entry_size, SEEK_SET) == 0);
	assert(fwrite(buf, 1, size, image) == size);
}

static u32 find_free_sector(void)
{
	static u32 start = 1;

	struct marfs_entry_header h;
	for (u32 i = start; i < header.entry_count - start; i++) {
		read_sector(i, sizeof(h), &h);
		if (h.type == MARFS_EMPTY_ENTRY) {
			start = i;
			return i;
		}
	}

	fprintf(stderr, "error: no free sector left\n");
	exit(1);
}

// first -> start = header.main.head
// TODO: Improve
static u8 read_entry_header(const char *path, u32 start, struct marfs_entry_header *entry_header)
{
	struct marfs_entry_header h;
	read_sector(start, sizeof(h), &h);

	if (*path == '/' && *(path + 1) == 0) {
		*entry_header = h;
		return 1;
	}

	while (*path && *path == '/')
		path++;

	if (h.type == MARFS_DIR_ENTRY) {
		struct marfs_dir_entry entry;
		read_sector(start, sizeof(entry), &entry);
		assert(entry.info.count <= MARFS_DIR_ENTRY_COUNT);
		for (u32 i = 0; i < entry.info.count; i++) {
			struct marfs_dir_entry_data *data = &entry.entries[i];
			if (!strncmp(path, data->name, data->length)) {
				if (path[data->length] == '/') {
					return read_entry_header(path, data->pointer.head,
								 entry_header);
				}
				if (!path[data->length]) {
					struct marfs_entry_header sub;
					read_sector(data->pointer.head, sizeof(sub), &sub);
					*entry_header = sub;
					return 1;
				}
			}
		}

		if (entry.info.header.next != MARFS_END)
			return read_entry_header(path, entry.info.header.next, entry_header);
		*entry_header = h;
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

	if (image) {
		write_sector(0, sizeof(header), &header);
		fclose(image);
	}
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

	struct marfs_entry_header h;
	u8 aah = read_entry_header(path, header.main.head, &h);
	if (!aah)
		return -ENOENT;

	if (h.type == MARFS_DIR_ENTRY) {
		stat->st_mode = S_IFDIR;
	} else if (h.type == MARFS_FILE_ENTRY) {
		struct marfs_file_entry_info i;
		read_sector(h.id, sizeof(i), &i);
		stat->st_mode = S_IFREG;
		stat->st_size = i.size;
	} else {
		return -EINVAL;
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

	struct marfs_entry_header h;
	u8 aah = read_entry_header(path, header.main.head, &h);
	if (!aah)
		return -ENOENT;

	if (h.type != MARFS_DIR_ENTRY)
		return -EINVAL;

	fill(buf, ".", NULL, 0, 0);
	fill(buf, "..", NULL, 0, 0);

	struct marfs_dir_entry d;
	u32 sector = h.id;
	do {
		read_sector(sector, sizeof(d), &d);
		for (u32 i = 0; i < d.info.count; i++)
			fill(buf, d.entries[i].name, NULL, 0, 0);
	} while ((sector = h.next) != MARFS_END);

	return 0;
}

static int marfs_read(const char *path, char *buf, size_t to_read, off_t offset,
		      struct fuse_file_info *file_info)
{
	debug("read() on %s, %lu", path, to_read);
	UNUSED(offset);
	UNUSED(file_info);

	struct marfs_entry_header h;
	u8 aah = read_entry_header(path, header.main.head, &h);
	if (!aah)
		return -ENOENT;

	if (h.type != MARFS_FILE_ENTRY)
		return -EISDIR;

	struct marfs_file_entry f;

	// TODO: Multiple sector read
	read_sector(h.id, sizeof(f), &f);
	to_read = MIN(to_read, f.info.size);
	memcpy(buf, f.data, MIN(sizeof(f.data), to_read));

	return to_read;
}

static int marfs_write(const char *path, const char *buf, size_t to_write, off_t offset,
		       struct fuse_file_info *file_info)
{
	debug("write() on %s", path);
	UNUSED(offset);
	UNUSED(file_info);

	struct marfs_entry_header h;
	u8 aah = read_entry_header(path, header.main.head, &h);
	if (!aah)
		return -ENOENT;

	if (h.type != MARFS_FILE_ENTRY)
		return -EISDIR;

	struct marfs_file_entry f;

	// TODO: Multiple sector write
	read_sector(h.id, sizeof(f), &f);
	memcpy(f.data, buf, MIN(sizeof(f.data), to_write));
	f.info.size = to_write; // TODO: Append??
	write_sector(h.id, sizeof(f), &f);

	return to_write;
}

static int marfs_create(const char *path, mode_t mode, struct fuse_file_info *file_info)
{
	debug("create() on %s", path);
	UNUSED(mode);
	UNUSED(file_info);

	struct marfs_entry_header parent = { .type = 0 };
	u8 aah = read_entry_header(path, header.main.head, &parent);
	if (aah) {
		debug("%s already exists\n", path);
		errno = EEXIST;
		return -1;
	}
	if (parent.type != MARFS_DIR_ENTRY) {
		debug("%s could not get created\n", path);
		return -1;
	}

	u32 sector = find_free_sector();
	struct marfs_file_entry_info info = {
		.header.type = MARFS_FILE_ENTRY,
		.header.id = sector,
		.header.prev = MARFS_END,
		.header.next = MARFS_END,
		.size = 0,
	};
	write_sector(sector, sizeof(info), &info);

	char *base = strrchr(path, '/');
	if (!(base++))
		return -EINVAL;

	u32 last = parent.id;
	while (parent.next != MARFS_END)
		last = parent.next;

	struct marfs_dir_entry dir;
	read_sector(last, sizeof(dir), &dir);

	assert(dir.info.count + 1 < MARFS_DIR_ENTRY_COUNT); // TODO: Create new dir entry at end

	struct marfs_dir_entry_data data = (struct marfs_dir_entry_data){
		.length = strlen(base),
		.pointer = MARFS_POINT(sector, sector),
	};

	if (data.length > MARFS_NAME_LENGTH)
		return -EINVAL;

	strcpy(data.name, base);
	dir.entries[dir.info.count++] = data;

	write_sector(last, sizeof(dir), &dir);

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

	image = fopen(path, "r+");
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
