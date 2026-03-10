#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

static char *skip_blanks(char *s) {
  if (s == NULL) {
    return NULL;
  }

  while (*s == ' ' || *s == '\t') {
    s ++;
  }

  return s;
}

static bool parse_positive_u64(char *s, uint64_t *out) {
  char *endptr;
  unsigned long long val;

  s = skip_blanks(s);
  if (s == NULL || *s == '\0') {
    return false;
  }

  val = strtoull(s, &endptr, 10);
  if (endptr == s || val == 0) {
    return false;
  }

  endptr = skip_blanks(endptr);
  if (*endptr != '\0') {
    return false;
  }

  *out = val;
  return true;
}

static bool parse_nonnegative_int(char *s, int *out) {
  char *endptr;
  long val;

  s = skip_blanks(s);
  if (s == NULL || *s == '\0') {
    return false;
  }

  val = strtol(s, &endptr, 10);
  if (endptr == s || val < 0) {
    return false;
  }

  endptr = skip_blanks(endptr);
  if (*endptr != '\0') {
    return false;
  }

  *out = (int)val;
  return true;
}

static void print_registers() {
  int i;

  for (i = 0; i < 8; i ++) {
    printf("%-4s\t0x%08x\t%u\n", regsl[i], reg_l(i), reg_l(i));
  }
  printf("%-4s\t0x%08x\t%u\n", "eip", cpu.eip, cpu.eip);
}

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args) {
  uint64_t n = 1;

  args = skip_blanks(args);
  if (args != NULL && *args != '\0') {
    if (!parse_positive_u64(args, &n)) {
      printf("Usage: si [N]\n");
      return 0;
    }
  }

  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  char *subcmd;

  subcmd = skip_blanks(args);
  if (subcmd == NULL || *subcmd == '\0') {
    printf("Usage: info r|w\n");
    return 0;
  }

  if (strcmp(subcmd, "r") == 0) {
    print_registers();
    return 0;
  }

  if (strcmp(subcmd, "w") == 0) {
    print_watchpoints();
    return 0;
  }

  printf("Unknown info subcommand '%s'\n", subcmd);
  return 0;
}

static int cmd_x(char *args) {
  char *expr_str;
  char *endptr;
  unsigned long n;
  unsigned long i;
  uint32_t addr;
  bool success = true;

  args = skip_blanks(args);
  if (args == NULL || *args == '\0') {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  n = strtoul(args, &endptr, 10);
  if (endptr == args || n == 0) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  expr_str = skip_blanks(endptr);
  if (*expr_str == '\0') {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  addr = expr(expr_str, &success);
  if (!success) {
    printf("Bad expression: %s\n", expr_str);
    return 0;
  }

  for (i = 0; i < n; i ++) {
    uint32_t cur_addr = addr + i * 4;
    uint32_t data = vaddr_read(cur_addr, 4);
    printf("0x%08x: 0x%08x\t%u\n", cur_addr, data, data);
  }

  return 0;
}

static int cmd_p(char *args) {
  uint32_t val;
  bool success = true;

  args = skip_blanks(args);
  if (args == NULL || *args == '\0') {
    printf("Usage: p EXPR\n");
    return 0;
  }

  val = expr(args, &success);
  if (!success) {
    printf("Bad expression: %s\n", args);
    return 0;
  }

  printf("%u (0x%x)\n", val, val);
  return 0;
}

static int cmd_w(char *args) {
  args = skip_blanks(args);
  if (args == NULL || *args == '\0') {
    printf("Usage: w EXPR\n");
    return 0;
  }

  new_watchpoint(args);
  return 0;
}

static int cmd_d(char *args) {
  int no;

  if (!parse_nonnegative_int(args, &no)) {
    printf("Usage: d N\n");
    return 0;
  }

  delete_watchpoint(no);
  return 0;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step through program by N instructions", cmd_si },
  { "info", "Print program status", cmd_info },
  { "x", "Examine memory", cmd_x },
  { "p", "Evaluate expression", cmd_p },
  { "w", "Set a watchpoint", cmd_w },
  { "d", "Delete a watchpoint", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = skip_blanks(args);
  int i;

  if (arg == NULL || *arg == '\0') {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
