#include "proc.h"
#include "memory.h"

static void *pf = NULL;

void* new_page(void) {
  assert(pf != NULL);
  assert(((uintptr_t)pf & PGMASK) == 0);
  assert((uintptr_t)pf + PGSIZE <= (uintptr_t)_heap.end);
  void *p = pf;
  pf = (void *)((uintptr_t)pf + PGSIZE);
  memset(p, 0, PGSIZE);
  return p;
}

void free_page(void *p) {
  assert(p != NULL);
  assert(((uintptr_t)p & PGMASK) == 0);
}

/* The brk() system call handler. */
int mm_brk(uint32_t new_brk) {
  if (current == NULL || new_brk == 0) {
    return 0;
  }

  if (current->max_brk == 0) {
    current->max_brk = PGROUNDUP(current->cur_brk);
  }

  uintptr_t old_max = PGROUNDUP(current->max_brk);
  uintptr_t new_max = PGROUNDUP(new_brk);

  for (uintptr_t va = old_max; va < new_max; va += PGSIZE) {
    _map(&current->as, (void *)va, new_page());
  }

  current->cur_brk = new_brk;
  if (new_max > current->max_brk) {
    current->max_brk = new_max;
  }
  return 0;
}

void init_mm() {
  pf = (void *)PGROUNDUP((uintptr_t)_heap.start);
  assert(((uintptr_t)pf & PGMASK) == 0);
  assert((uintptr_t)pf <= (uintptr_t)_heap.end);
  Log("free physical pages starting from %p", pf);

  _pte_init(new_page, free_page);
}
