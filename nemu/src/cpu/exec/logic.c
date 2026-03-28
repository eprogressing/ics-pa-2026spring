#include "cpu/exec.h"

static inline uint32_t width_mask(int width) {
  switch (width) {
    case 1: return 0xff;
    case 2: return 0xffff;
    case 4: return 0xffffffff;
    default: assert(0);
  }
}

static inline void rtl_trunc(rtlreg_t *dest, int width) {
  *dest &= width_mask(width);
}

static inline rtlreg_t sign_ext_val(rtlreg_t val, int width) {
  switch (width) {
    case 1: return (int32_t)(int8_t)val;
    case 2: return (int32_t)(int16_t)val;
    case 4: return val;
    default: assert(0);
  }
}

static inline void update_logic_flags(const rtlreg_t *result, int width) {
  rtlreg_t zero = 0;
  rtl_update_ZFSF(result, width);
  rtl_set_CF(&zero);
  rtl_set_OF(&zero);
}

make_EHelper(test) {
  rtl_and(&t2, &id_dest->val, &id_src->val);
  rtl_trunc(&t2, id_dest->width);
  update_logic_flags(&t2, id_dest->width);

  print_asm_template2(test);
}

make_EHelper(and) {
  rtl_and(&t2, &id_dest->val, &id_src->val);
  rtl_trunc(&t2, id_dest->width);
  operand_write(id_dest, &t2);
  update_logic_flags(&t2, id_dest->width);

  print_asm_template2(and);
}

make_EHelper(xor) {
  rtl_xor(&t2, &id_dest->val, &id_src->val);
  rtl_trunc(&t2, id_dest->width);
  operand_write(id_dest, &t2);
  update_logic_flags(&t2, id_dest->width);

  print_asm_template2(xor);
}

make_EHelper(or) {
  rtl_or(&t2, &id_dest->val, &id_src->val);
  rtl_trunc(&t2, id_dest->width);
  operand_write(id_dest, &t2);
  update_logic_flags(&t2, id_dest->width);

  print_asm_template2(or);
}

make_EHelper(rol) {
  uint32_t bits = id_dest->width * 8;
  uint32_t count = (id_src->val & 0x1f) % bits;
  rtlreg_t orig = id_dest->val & width_mask(id_dest->width);

  if (count != 0) {
    rtlreg_t result = ((orig << count) | (orig >> (bits - count))) & width_mask(id_dest->width);
    rtl_li(&t2, result);
    operand_write(id_dest, &t2);

    rtl_li(&t0, result & 0x1);
    rtl_set_CF(&t0);

    if (count == 1) {
      rtl_msb(&t0, &t2, id_dest->width);
      rtl_get_CF(&t1);
      rtl_xor(&t0, &t0, &t1);
      rtl_set_OF(&t0);
    }
  }

  print_asm_template2(rol);
}

make_EHelper(ror) {
  uint32_t bits = id_dest->width * 8;
  uint32_t count = (id_src->val & 0x1f) % bits;
  rtlreg_t orig = id_dest->val & width_mask(id_dest->width);

  if (count != 0) {
    rtlreg_t result = ((orig >> count) | (orig << (bits - count))) & width_mask(id_dest->width);
    rtl_li(&t2, result);
    operand_write(id_dest, &t2);

    rtl_msb(&t0, &t2, id_dest->width);
    rtl_set_CF(&t0);

    if (count == 1) {
      rtl_li(&t0, (result >> (bits - 1)) & 0x1);
      rtl_li(&t1, (result >> (bits - 2)) & 0x1);
      rtl_xor(&t0, &t0, &t1);
      rtl_set_OF(&t0);
    }
  }

  print_asm_template2(ror);
}

make_EHelper(sar) {
  uint32_t count = id_src->val & 0x1f;
  uint32_t bits = id_dest->width * 8;
  rtlreg_t orig = id_dest->val & width_mask(id_dest->width);

  if (count != 0) {
    rtl_li(&t0, orig);
    rtl_li(&t1, sign_ext_val(orig, id_dest->width));
    rtl_sari(&t2, &t1, count);
    rtl_trunc(&t2, id_dest->width);
    operand_write(id_dest, &t2);

    if (count <= bits) {
      rtl_li(&t0, (orig >> (count - 1)) & 0x1);
      rtl_set_CF(&t0);
    }

    rtl_update_ZFSF(&t2, id_dest->width);

    if (count == 1) {
      rtl_li(&t0, 0);
      rtl_set_OF(&t0);
    }
  }

  print_asm_template2(sar);
}

make_EHelper(shl) {
  uint32_t count = id_src->val & 0x1f;
  uint32_t bits = id_dest->width * 8;
  rtlreg_t orig = id_dest->val & width_mask(id_dest->width);

  if (count != 0) {
    rtl_li(&t0, orig);
    rtl_shli(&t2, &t0, count);
    rtl_trunc(&t2, id_dest->width);
    operand_write(id_dest, &t2);

    if (count <= bits) {
      rtl_li(&t0, (orig >> (bits - count)) & 0x1);
      rtl_set_CF(&t0);
    }

    rtl_update_ZFSF(&t2, id_dest->width);

    if (count == 1) {
      rtl_msb(&t0, &t2, id_dest->width);
      rtl_get_CF(&t1);
      rtl_xor(&t0, &t0, &t1);
      rtl_set_OF(&t0);
    }
  }

  print_asm_template2(shl);
}

make_EHelper(shr) {
  uint32_t count = id_src->val & 0x1f;
  uint32_t bits = id_dest->width * 8;
  rtlreg_t orig = id_dest->val & width_mask(id_dest->width);

  if (count != 0) {
    rtl_li(&t0, orig);
    rtl_shri(&t2, &t0, count);
    rtl_trunc(&t2, id_dest->width);
    operand_write(id_dest, &t2);

    if (count <= bits) {
      rtl_li(&t0, (orig >> (count - 1)) & 0x1);
      rtl_set_CF(&t0);
    }

    rtl_update_ZFSF(&t2, id_dest->width);

    if (count == 1) {
      rtl_li(&t0, (orig >> (bits - 1)) & 0x1);
      rtl_set_OF(&t0);
    }
  }

  print_asm_template2(shr);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  rtl_mv(&t2, &id_dest->val);
  rtl_not(&t2);
  rtl_trunc(&t2, id_dest->width);
  operand_write(id_dest, &t2);

  print_asm_template1(not);
}
