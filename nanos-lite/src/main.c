#include "common.h"

/* Uncomment these macros to enable corresponding functionality. */
#define HAS_ASYE
#define HAS_PTE

void init_mm(void);
void init_ramdisk(void);
void init_device(void);
void init_irq(void);
void init_fs(void);
uint32_t loader(_Protect *, const char *);

static const char *init_program = "/bin/pal";

int main() {
#ifdef HAS_PTE
  init_mm();
#endif

  Log("'Hello World!' from Nanos-lite");
  Log("Build time: %s, %s", __TIME__, __DATE__);

  init_ramdisk();

  init_device();

#ifdef HAS_ASYE
  Log("Initializing interrupt/exception handler...");
  init_irq();
#endif

  init_fs();

  uint32_t entry = loader(NULL, init_program);
  ((void (*)(void))entry)();

  panic("Should not reach here");
}
