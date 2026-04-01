#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

extern int fs_open(const char *pathname, int flags, int mode);
extern size_t fs_read(int fd, void *buf, size_t len);
extern int fs_close(int fd);
extern size_t fs_filesz(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  (void)as;

  if (filename == NULL || filename[0] == '\0') {
    filename = "/bin/hello";
  }

  int fd = fs_open(filename, 0, 0);
  size_t size = fs_filesz(fd);
  size_t nread = fs_read(fd, DEFAULT_ENTRY, size);
  assert(nread == size);
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
