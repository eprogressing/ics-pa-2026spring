#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t save_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  rtl_li(&t0, cpu.eflags);
  rtl_push(&t0);

  rtl_li(&t0, cpu.cs);
  rtl_push(&t0);

  rtl_li(&t0, save_addr);
  rtl_push(&t0);

  vaddr_t idt_addr = cpu.idtr.base + NO * 8;
  uint32_t gate_lo = vaddr_read(idt_addr, 4);
  uint32_t gate_hi = vaddr_read(idt_addr + 4, 4);

  cpu.cs = gate_lo >> 16;
  decoding.jmp_eip = (gate_lo & 0xffff) | (gate_hi & 0xffff0000);
  decoding.is_jmp = 1;
}

void dev_raise_intr() {
}
