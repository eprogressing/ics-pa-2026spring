#include "common.h"
#include "memory.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

uintptr_t loader_brk = 0;

extern int fs_open(const char *pathname, int flags, int mode);
extern size_t fs_read(int fd, void *buf, size_t len);
extern int fs_close(int fd);
extern size_t fs_filesz(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  if (filename == NULL || filename[0] == '\0') {
    filename = "/bin/hello";
  }

  int fd = fs_open(filename, 0, 0);
  size_t size = fs_filesz(fd);
  loader_brk = PGROUNDUP((uintptr_t)DEFAULT_ENTRY + size);

  if (as == NULL) {
    size_t nread = fs_read(fd, DEFAULT_ENTRY, size);
    assert(nread == size);
  }
  else {
    uintptr_t va = (uintptr_t)DEFAULT_ENTRY;
    size_t offset = 0;

    while (offset < size) {
      void *pa = new_page();
      _map(as, (void *)(va + offset), pa);

      size_t len = PGSIZE;
      if (offset + len > size) {
        len = size - offset;
      }

      size_t nread = fs_read(fd, pa, len);
      assert(nread == len);
      offset += len;
    }
  }

  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
