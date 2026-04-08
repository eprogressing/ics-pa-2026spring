#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

#define KEYDOWN_MASK 0x8000

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

static char dispinfo[128] __attribute__((used));
static size_t dispinfo_len = 0;

size_t events_read(void *buf, size_t len) {
  if (len == 0) {
    return 0;
  }

  char event[64];
  int key = _read_key();
  if (key != _KEY_NONE) {
    bool keydown = (key & KEYDOWN_MASK) != 0;
    int code = key & ~KEYDOWN_MASK;
    const char *name = keyname[code];
    if (name == NULL) {
      name = "NONE";
    }
    sprintf(event, "%s %s\n", keydown ? "kd" : "ku", name);
  }
  else {
    sprintf(event, "t %d\n", (int)_uptime());
  }

  size_t nread = strlen(event);
  if (nread > len) {
    nread = len;
  }
  memcpy(buf, event, nread);
  return nread;
}

size_t dispinfo_read(void *buf, off_t offset, size_t len) {
  if (offset < 0 || (size_t)offset >= dispinfo_len) {
    return 0;
  }

  size_t nread = len;
  if ((size_t)offset + nread > dispinfo_len) {
    nread = dispinfo_len - (size_t)offset;
  }
  memcpy(buf, dispinfo + offset, nread);
  return nread;
}

size_t fb_write(const void *buf, off_t offset, size_t len) {
  if (offset < 0) {
    return 0;
  }

  size_t screen_size = _screen.width * _screen.height * sizeof(uint32_t);
  if ((size_t)offset >= screen_size) {
    return 0;
  }

  size_t nwrite = len;
  if ((size_t)offset + nwrite > screen_size) {
    nwrite = screen_size - (size_t)offset;
  }

  nwrite -= nwrite % sizeof(uint32_t);
  if (nwrite == 0) {
    return 0;
  }

  assert(offset % sizeof(uint32_t) == 0);

  const uint32_t *pixels = (const uint32_t *)buf;
  size_t pixel_offset = (size_t)offset / sizeof(uint32_t);
  size_t nr_pixel = nwrite / sizeof(uint32_t);

  while (nr_pixel > 0) {
    int x = pixel_offset % _screen.width;
    int y = pixel_offset / _screen.width;
    int row_pixel = _screen.width - x;
    if ((size_t)row_pixel > nr_pixel) {
      row_pixel = nr_pixel;
    }

    _draw_rect(pixels, x, y, row_pixel, 1);

    pixels += row_pixel;
    pixel_offset += row_pixel;
    nr_pixel -= row_pixel;
  }

  _draw_sync();
  return nwrite;
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
  dispinfo_len = strlen(dispinfo);
}
