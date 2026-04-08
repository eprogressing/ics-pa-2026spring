#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  size_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);
extern size_t dispinfo_read(void *buf, off_t offset, size_t len);
extern size_t fb_write(const void *buf, off_t offset, size_t len);

static inline Finfo *get_file(int fd) {
  if ((size_t)fd >= NR_FILES) {
    assert(0);
  }
  return (Finfo *)((char *)file_table + (size_t)fd * sizeof(Finfo));
}

void init_fs() {
  file_table[FD_FB].size = _screen.width * _screen.height * sizeof(uint32_t);

  size_t i;
  for (i = 0; i < NR_FILES; i ++) {
    file_table[i].open_offset = 0;
  }
}

int fs_open(const char *pathname, int flags, int mode) {
  (void)flags;
  (void)mode;

  size_t i;
  for (i = 0; i < NR_FILES; i ++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return (int)i;
    }
  }

  assert(0);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  Finfo *f = get_file(fd);

  switch (fd) {
    case FD_STDIN:
    case FD_STDOUT:
    case FD_STDERR:
    case FD_FB:
      assert(0);
      return 0;
    case FD_EVENTS:
      return events_read(buf, len);
    case FD_DISPINFO: {
      size_t nread = dispinfo_read(buf, f->open_offset, len);
      f->open_offset += nread;
      return nread;
    }
    default: {
      assert(f->open_offset <= f->size);
      size_t nread = len;
      if (f->open_offset + nread > f->size) {
        nread = f->size - f->open_offset;
      }
      ramdisk_read(buf, f->disk_offset + f->open_offset, nread);
      f->open_offset += nread;
      return nread;
    }
  }
}

size_t fs_write(int fd, const void *buf, size_t len) {
  Finfo *f = get_file(fd);

  switch (fd) {
    case FD_STDOUT:
    case FD_STDERR: {
      const char *str = (const char *)buf;
      size_t i;
      for (i = 0; i < len; i ++) {
        _putc(str[i]);
      }
      return len;
    }
    case FD_STDIN:
    case FD_EVENTS:
    case FD_DISPINFO:
      assert(0);
      return 0;
    case FD_FB: {
      assert(f->open_offset <= f->size);
      size_t nwrite = fb_write(buf, f->open_offset, len);
      f->open_offset += nwrite;
      return nwrite;
    }
    default: {
      assert(f->open_offset <= f->size);
      size_t nwrite = len;
      if (f->open_offset + nwrite > f->size) {
        nwrite = f->size - f->open_offset;
      }
      ramdisk_write(buf, f->disk_offset + f->open_offset, nwrite);
      f->open_offset += nwrite;
      return nwrite;
    }
  }
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  Finfo *f = get_file(fd);
  size_t new_offset = 0;

  switch (whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = f->open_offset + offset;
      break;
    case SEEK_END:
      new_offset = f->size + offset;
      break;
    default:
      assert(0);
  }

  assert(new_offset <= f->size);
  f->open_offset = new_offset;
  return new_offset;
}

int fs_close(int fd) {
  get_file(fd);
  return 0;
}

size_t fs_filesz(int fd) {
  return get_file(fd)->size;
}
