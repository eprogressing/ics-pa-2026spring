#include "nemu.h"
#include "device/mmio.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

static inline bool is_page_enabled() {
  return cpu.cr0.paging;
}

static paddr_t page_translate(vaddr_t addr, bool is_write) {
  uint32_t dir_idx = addr >> 22;
  uint32_t tab_idx = (addr >> 12) & 0x3ff;
  uint32_t page_off = addr & PAGE_MASK;

  paddr_t pdir_base = cpu.cr3.page_directory_base << 12;
  paddr_t pde_addr = pdir_base + dir_idx * sizeof(PDE);
  PDE pde;
  pde.val = paddr_read(pde_addr, 4);
  assert(pde.present);
  pde.accessed = 1;
  paddr_write(pde_addr, 4, pde.val);

  paddr_t ptab_base = pde.page_frame << 12;
  paddr_t pte_addr = ptab_base + tab_idx * sizeof(PTE);
  PTE pte;
  pte.val = paddr_read(pte_addr, 4);
  assert(pte.present);
  pte.accessed = 1;
  if (is_write) {
    pte.dirty = 1;
  }
  paddr_write(pte_addr, 4, pte.val);

  return (pte.page_frame << 12) | page_off;
}

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    return mmio_read(addr, len, map_NO);
  }
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    mmio_write(addr, len, data, map_NO);
    return;
  }
  memcpy(guest_to_host(addr), &data, len);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if (is_page_enabled()) {
    if (((addr & PAGE_MASK) + len - 1) >= PAGE_SIZE) {
      uint32_t data = 0;
      for (int i = 0; i < len; i ++) {
        data |= paddr_read(page_translate(addr + i, false), 1) << (i << 3);
      }
      return data;
    }
    return paddr_read(page_translate(addr, false), len);
  }
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (is_page_enabled()) {
    if (((addr & PAGE_MASK) + len - 1) >= PAGE_SIZE) {
      for (int i = 0; i < len; i ++) {
        paddr_write(page_translate(addr + i, true), 1, data >> (i << 3));
      }
      return;
    }
    paddr_write(page_translate(addr, true), len, data);
    return;
  }
  paddr_write(addr, len, data);
}
