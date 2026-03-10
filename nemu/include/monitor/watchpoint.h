#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

#define WP_EXPR_LEN 128

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char expr[WP_EXPR_LEN];
  uint32_t last_val;

} WP;

void init_wp_pool(void);
WP *new_wp(void);
void free_wp(WP *wp);
WP *new_watchpoint(char *expr);
bool delete_watchpoint(int no);
void print_watchpoints(void);
bool check_watchpoints(void);

#endif
