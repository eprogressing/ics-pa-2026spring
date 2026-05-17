#include "proc.h"

#define MAX_NR_PROC 4
#define USER_STACK_TOP 0x10000000

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);
extern uintptr_t loader_brk;

static void switch_to_user(_RegSet *tf) __attribute__((noreturn));
static void switch_to_user(_RegSet *tf) {
  asm volatile(
      "movl %0, %%esp;"
      "popal;"
      "addl $8, %%esp;"
      "iret"
      :
      : "r"(tf)
      : "memory");
  while (1);
}

void load_prog(const char *filename) {
  int i = nr_proc;
  assert(i < MAX_NR_PROC);
  nr_proc ++;

  memset(&pcb[i], 0, sizeof(PCB));
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);
  pcb[i].cur_brk = loader_brk;
  pcb[i].max_brk = loader_brk;

  _Area stack;
  stack.start = (void *)(USER_STACK_TOP - STACK_SIZE);
  stack.end = (void *)USER_STACK_TOP;

  _Area kstack;
  kstack.start = pcb[i].stack;
  kstack.end = pcb[i].stack + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, kstack, (void *)entry, NULL, NULL);
  current = &pcb[i];

  _switch(&pcb[i].as);
  switch_to_user(pcb[i].tf);
}

_RegSet* schedule(_RegSet *prev) {
  if (current != NULL && prev != NULL) {
    current->tf = prev;
  }
  assert(current != NULL);
  _switch(&current->as);
  return current->tf;
}
