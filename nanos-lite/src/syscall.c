#include "common.h"
#include "syscall.h"

extern int fs_open(const char *pathname, int flags, int mode);
extern size_t fs_read(int fd, void *buf, size_t len);
extern size_t fs_write(int fd, const void *buf, size_t len);
extern size_t fs_lseek(int fd, size_t offset, int whence);
extern int fs_close(int fd);
extern int mm_brk(uint32_t new_brk);

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  uintptr_t ret = 0;

  switch (a[0]) {
    case SYS_none:
      ret = 1;
      break;
    case SYS_open:
      ret = fs_open((const char *)a[1], a[2], a[3]);
      break;
    case SYS_read:
      ret = fs_read(a[1], (void *)a[2], a[3]);
      break;
    case SYS_write:
      ret = fs_write(a[1], (const void *)a[2], a[3]);
      break;
    case SYS_exit:
      _halt(a[1]);
      break;
    case SYS_close:
      ret = fs_close(a[1]);
      break;
    case SYS_lseek:
      ret = fs_lseek(a[1], a[2], a[3]);
      break;
    case SYS_brk:
      ret = mm_brk(a[1]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  SYSCALL_ARG1(r) = ret;
  return r;
}
