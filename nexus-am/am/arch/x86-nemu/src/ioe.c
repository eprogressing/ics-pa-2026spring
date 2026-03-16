#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64
#define KBD_HASKEY_MASK 0x1
#define KEYDOWN_MASK 0x8000
#define FB_ADDR 0x40000
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  return inl(RTC_PORT) - boot_time;
}

volatile uint32_t* const fb = (volatile uint32_t *)FB_ADDR;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  int src_x = 0, src_y = 0;
  int src_w = w;
  int row, col;

  if (pixels == NULL || w <= 0 || h <= 0) {
    return;
  }

  if (x < 0) {
    src_x = -x;
    w += x;
    x = 0;
  }
  if (y < 0) {
    src_y = -y;
    h += y;
    y = 0;
  }
  if (x >= _screen.width || y >= _screen.height) {
    return;
  }
  if (x + w > _screen.width) {
    w = _screen.width - x;
  }
  if (y + h > _screen.height) {
    h = _screen.height - y;
  }
  if (w <= 0 || h <= 0) {
    return;
  }

  for (row = 0; row < h; row ++) {
    int dst_off = (y + row) * _screen.width + x;
    int src_off = (src_y + row) * src_w + src_x;
    for (col = 0; col < w; col ++) {
      fb[dst_off + col] = pixels[src_off + col];
    }
  }
}

void _draw_sync() {
}

int _read_key() {
  if ((inb(KBD_STATUS_PORT) & KBD_HASKEY_MASK) == 0) {
    return _KEY_NONE;
  }

  return inl(KBD_DATA_PORT) & (KEYDOWN_MASK | 0x7fff);
}
