#include <am.h>
#include <x86.h>

#define PG_ALIGN __attribute((aligned(PGSIZE)))
#define USER_SPACE_START 0x08000000
#define USER_SPACE_END   0x10000000

static PDE kpdirs[NR_PDE] PG_ALIGN;
static PTE kptabs[PMEM_SIZE / PGSIZE] PG_ALIGN;
static void* (*palloc_f)();
static void (*pfree_f)(void*);

_Area segments[] = {      // Kernel memory mappings
  {.start = (void*)0,          .end = (void*)PMEM_SIZE}
};

#define NR_KSEG_MAP (sizeof(segments) / sizeof(segments[0]))

void _pte_init(void* (*palloc)(), void (*pfree)(void*)) {
  palloc_f = palloc;
  pfree_f = pfree;

  int i;

  // make all PDEs invalid
  for (i = 0; i < NR_PDE; i ++) {
    kpdirs[i] = 0;
  }

  PTE *ptab = kptabs;
  for (i = 0; i < NR_KSEG_MAP; i ++) {
    uint32_t pdir_idx = (uintptr_t)segments[i].start / (PGSIZE * NR_PTE);
    uint32_t pdir_idx_end = (uintptr_t)segments[i].end / (PGSIZE * NR_PTE);
    for (; pdir_idx < pdir_idx_end; pdir_idx ++) {
      // fill PDE
      kpdirs[pdir_idx] = (uintptr_t)ptab | PTE_P;

      // fill PTE
      PTE pte = PGADDR(pdir_idx, 0, 0) | PTE_P;
      PTE pte_end = PGADDR(pdir_idx + 1, 0, 0) | PTE_P;
      for (; pte < pte_end; pte += PGSIZE) {
        *ptab = pte;
        ptab ++;
      }
    }
  }

  set_cr3(kpdirs);
  set_cr0(get_cr0() | CR0_PG);
}

void _protect(_Protect *p) {
  PDE *updir = (PDE*)(palloc_f());
  p->ptr = updir;
  // map kernel space
  for (int i = 0; i < NR_PDE; i ++) {
    if (i < PDX(USER_SPACE_START) || i >= PDX(USER_SPACE_END)) {
      updir[i] = kpdirs[i];
    }
  }

  p->area.start = (void*)USER_SPACE_START;
  p->area.end = (void*)USER_SPACE_END;
}

void _release(_Protect *p) {
}

void _switch(_Protect *p) {
  set_cr3(p->ptr);
}

void _map(_Protect *p, void *va, void *pa) {
  if ((uintptr_t)va % PGSIZE != 0) _halt(1);
  if ((uintptr_t)pa % PGSIZE != 0) _halt(1);
  if (!(p->area.start <= va && va < p->area.end)) _halt(1);

  PDE *updir = (PDE *)p->ptr;
  uint32_t pdx = PDX(va);
  uint32_t ptx = PTX(va);

  if ((updir[pdx] & PTE_P) == 0) {
    PTE *ptab = (PTE *)palloc_f();
    updir[pdx] = (uintptr_t)ptab | PTE_P | PTE_W | PTE_U;
  }

  PTE *ptab = (PTE *)PTE_ADDR(updir[pdx]);
  if (ptab[ptx] & PTE_P) _halt(1);
  ptab[ptx] = (uintptr_t)pa | PTE_P | PTE_W | PTE_U;
}

void _unmap(_Protect *p, void *va) {
}

_RegSet *_umake(_Protect *p, _Area ustack, _Area kstack, void *entry, char *const argv[], char *const envp[]) {
  (void)argv;
  (void)envp;

  uintptr_t start = (uintptr_t)ustack.start;
  uintptr_t end = (uintptr_t)ustack.end;
  if (start % PGSIZE != 0 || end % PGSIZE != 0 || start >= end) _halt(1);

  for (uintptr_t va = start; va < end; va += PGSIZE) {
    _map(p, (void *)va, palloc_f());
  }

  _RegSet *tf = (_RegSet *)((uintptr_t)kstack.end - sizeof(_RegSet));
  uintptr_t *slot = (uintptr_t *)tf;
  for (size_t i = 0; i < sizeof(_RegSet) / sizeof(uintptr_t); i ++) {
    slot[i] = 0;
  }

  tf->esp = (uintptr_t)ustack.end;
  tf->eip = (uintptr_t)entry;
  tf->cs = USEL(SEG_UCODE);
  tf->eflags = 0x2;
  return tf;
}
