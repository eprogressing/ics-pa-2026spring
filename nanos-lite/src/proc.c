#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc;
  assert(i < MAX_NR_PROC);
  nr_proc ++;

  memset(&pcb[i], 0, sizeof(PCB));
  _protect(&pcb[i].as);
  pcb[i].cur_brk = 0;
  pcb[i].max_brk = 0;

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  _switch(&pcb[i].as);
  current = &pcb[i];
  ((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

_RegSet* schedule(_RegSet *prev) {
  return NULL;
}
