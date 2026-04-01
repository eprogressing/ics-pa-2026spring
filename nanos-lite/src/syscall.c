#include "common.h"
#include "syscall.h"

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
    case SYS_write: {
      int fd = a[1];
      const char *buf = (const char *)a[2];
      size_t len = a[3];
      size_t i;
      switch (fd) {
        case 1:
        case 2:
          for (i = 0; i < len; i ++) {
            _putc(buf[i]);
          }
          ret = len;
          break;
        default:
          panic("Unhandled fd = %d", fd);
      }
      break;
    }
    case SYS_exit:
      _halt(a[1]);
      break;
    case SYS_brk:
      ret = 0;
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  SYSCALL_ARG1(r) = ret;
  return r;
}
