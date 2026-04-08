#include "cpu/exec.h"

make_EHelper(mov) {
  operand_write(id_dest, &id_src->val);
  print_asm_template2(mov);
}

make_EHelper(movsb) {
  rtl_lr_l(&t0, R_ESI);
  rtl_lm(&t1, &t0, 1);

  rtl_lr_l(&t2, R_EDI);
  rtl_sm(&t2, 1, &t1);

  int step = (cpu.eflags & 0x400) ? -1 : 1;
  rtl_addi(&t0, &t0, step);
  rtl_sr_l(R_ESI, &t0);
  rtl_addi(&t2, &t2, step);
  rtl_sr_l(R_EDI, &t2);

  print_asm("movsb");
}

make_EHelper(push) {
  if (id_dest->type == OP_TYPE_IMM && decoding.opcode == 0x6a) {
    t1 = (int32_t)(int8_t)id_dest->val;
  }
  else {
    t1 = id_dest->val;
  }

  rtl_lr_l(&t0, R_ESP);
  rtl_addi(&t0, &t0, -id_dest->width);
  rtl_sr_l(R_ESP, &t0);
  rtl_sm(&t0, id_dest->width, &t1);

  print_asm_template1(push);
}

make_EHelper(pop) {
  rtl_lr_l(&t0, R_ESP);
  rtl_lm(&t1, &t0, id_dest->width);
  rtl_addi(&t0, &t0, id_dest->width);
  rtl_sr_l(R_ESP, &t0);
  operand_write(id_dest, &t1);

  print_asm_template1(pop);
}

make_EHelper(pusha) {
  rtl_lr_l(&t0, R_ESP);

  rtl_lr_l(&t1, R_EAX);
  rtl_push(&t1);
  rtl_lr_l(&t1, R_ECX);
  rtl_push(&t1);
  rtl_lr_l(&t1, R_EDX);
  rtl_push(&t1);
  rtl_lr_l(&t1, R_EBX);
  rtl_push(&t1);
  rtl_push(&t0);
  rtl_lr_l(&t1, R_EBP);
  rtl_push(&t1);
  rtl_lr_l(&t1, R_ESI);
  rtl_push(&t1);
  rtl_lr_l(&t1, R_EDI);
  rtl_push(&t1);

  print_asm("pusha");
}

make_EHelper(popa) {
  rtl_pop(&t0);
  rtl_sr_l(R_EDI, &t0);
  rtl_pop(&t0);
  rtl_sr_l(R_ESI, &t0);
  rtl_pop(&t0);
  rtl_sr_l(R_EBP, &t0);
  rtl_pop(&t0);
  rtl_pop(&t0);
  rtl_sr_l(R_EBX, &t0);
  rtl_pop(&t0);
  rtl_sr_l(R_EDX, &t0);
  rtl_pop(&t0);
  rtl_sr_l(R_ECX, &t0);
  rtl_pop(&t0);
  rtl_sr_l(R_EAX, &t0);

  print_asm("popa");
}

make_EHelper(leave) {
  rtl_lr_l(&t0, R_EBP);
  rtl_sr_l(R_ESP, &t0);
  rtl_lm(&t1, &t0, id_dest->width);
  rtl_addi(&t0, &t0, id_dest->width);
  rtl_sr_l(R_ESP, &t0);
  rtl_sr(R_EBP, id_dest->width, &t1);

  print_asm("leave");
}

make_EHelper(cltd) {
  if (decoding.is_operand_size_16) {
    rtl_lr_w(&t0, R_AX);
    rtl_li(&t1, ((int16_t)t0 < 0) ? 0xffff : 0);
    rtl_sr_w(R_DX, &t1);
  }
  else {
    rtl_lr_l(&t0, R_EAX);
    rtl_li(&t1, ((int32_t)t0 < 0) ? 0xffffffff : 0);
    rtl_sr_l(R_EDX, &t1);
  }

  print_asm(decoding.is_operand_size_16 ? "cwtl" : "cltd");
}

make_EHelper(cwtl) {
  if (decoding.is_operand_size_16) {
    rtl_lr_b(&t0, R_AL);
    rtl_li(&t1, (int16_t)(int8_t)t0);
    rtl_sr_w(R_AX, &t1);
  }
  else {
    rtl_lr_w(&t0, R_AX);
    rtl_li(&t1, (int32_t)(int16_t)t0);
    rtl_sr_l(R_EAX, &t1);
  }

  print_asm(decoding.is_operand_size_16 ? "cbtw" : "cwtl");
}

make_EHelper(movsx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;

  switch (id_src->width) {
    case 1: rtl_li(&t2, (int32_t)(int8_t)id_src->val); break;
    case 2: rtl_li(&t2, (int32_t)(int16_t)id_src->val); break;
    default: assert(0);
  }

  operand_write(id_dest, &t2);
  print_asm_template2(movsx);
}

make_EHelper(movzx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;

  switch (id_src->width) {
    case 1: rtl_li(&t2, (uint8_t)id_src->val); break;
    case 2: rtl_li(&t2, (uint16_t)id_src->val); break;
    default: assert(0);
  }

  operand_write(id_dest, &t2);
  print_asm_template2(movzx);
}

make_EHelper(lea) {
  rtl_li(&t2, id_src->addr);
  operand_write(id_dest, &t2);
  print_asm_template2(lea);
}
