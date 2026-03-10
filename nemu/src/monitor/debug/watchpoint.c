#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

static WP *find_wp(int no) {
  WP *p = head;

  while (p != NULL) {
    if (p->NO == no) {
      return p;
    }
    p = p->next;
  }

  return NULL;
}

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].expr[0] = '\0';
    wp_pool[i].last_val = 0;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

WP *new_wp() {
  WP *wp;

  assert(free_ != NULL);

  wp = free_;
  free_ = free_->next;

  wp->expr[0] = '\0';
  wp->last_val = 0;
  wp->next = head;
  head = wp;

  return wp;
}

void free_wp(WP *wp) {
  WP *prev = NULL;
  WP *cur = head;

  assert(wp != NULL);

  while (cur != NULL && cur != wp) {
    prev = cur;
    cur = cur->next;
  }

  assert(cur != NULL);

  if (prev == NULL) {
    head = cur->next;
  }
  else {
    prev->next = cur->next;
  }

  cur->expr[0] = '\0';
  cur->last_val = 0;
  cur->next = free_;
  free_ = cur;
}

WP *new_watchpoint(char *e) {
  WP *wp;
  bool success = true;
  uint32_t val;
  size_t len;

  if (e == NULL) {
    printf("Watchpoint expression is empty.\n");
    return NULL;
  }

  len = strlen(e);
  if (len == 0) {
    printf("Watchpoint expression is empty.\n");
    return NULL;
  }

  if (len >= WP_EXPR_LEN) {
    printf("Watchpoint expression is too long.\n");
    return NULL;
  }

  val = expr(e, &success);
  if (!success) {
    printf("Failed to create watchpoint: bad expression \"%s\".\n", e);
    return NULL;
  }

  wp = new_wp();
  memcpy(wp->expr, e, len + 1);
  wp->last_val = val;

  printf("Watchpoint %d: %s = %u (0x%x)\n", wp->NO, wp->expr, wp->last_val, wp->last_val);
  return wp;
}

bool delete_watchpoint(int no) {
  WP *wp = find_wp(no);

  if (wp == NULL) {
    printf("Watchpoint %d not found.\n", no);
    return false;
  }

  printf("Delete watchpoint %d: %s\n", wp->NO, wp->expr);
  free_wp(wp);
  return true;
}

void print_watchpoints() {
  WP *p = head;

  if (p == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num\tWhat\tValue\n");
  while (p != NULL) {
    printf("%d\t%s\t%u (0x%x)\n", p->NO, p->expr, p->last_val, p->last_val);
    p = p->next;
  }
}

bool check_watchpoints() {
  WP *p = head;

  while (p != NULL) {
    bool success = true;
    uint32_t new_val = expr(p->expr, &success);

    if (!success) {
      printf("Failed to evaluate watchpoint %d: %s\n", p->NO, p->expr);
      return false;
    }

    if (new_val != p->last_val) {
      printf("Watchpoint %d triggered: %s\n", p->NO, p->expr);
      printf("Old value = %u (0x%x)\n", p->last_val, p->last_val);
      printf("New value = %u (0x%x)\n", new_val, new_val);
      p->last_val = new_val;
      return true;
    }

    p = p->next;
  }

  return false;
}

