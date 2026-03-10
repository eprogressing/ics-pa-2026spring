#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_DEC, TK_HEX, TK_REG, TK_NEQ, TK_AND, TK_DEREF

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
  {"0[xX][0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_DEC},
  {"\\$[a-zA-Z][a-zA-Z0-9]*", TK_REG},
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},       // and
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply or dereference
  {"/", '/'}            // divide
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool is_value_token(int type) {
  return type == TK_DEC || type == TK_HEX || type == TK_REG || type == ')';
}

static uint32_t parse_u32(const char *s, int base, bool *success) {
  uint32_t val = 0;
  int i = 0;

  if (base == 16) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
      i = 2;
    }
  }
  else if (base != 10) {
    *success = false;
    return 0;
  }

  if (s[i] == '\0') {
    *success = false;
    return 0;
  }

  for (; s[i] != '\0'; i ++) {
    int digit;
    char c = s[i];

    if (c >= '0' && c <= '9') {
      digit = c - '0';
    }
    else if (c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    }
    else if (c >= 'A' && c <= 'F') {
      digit = c - 'A' + 10;
    }
    else {
      *success = false;
      return 0;
    }

    if (digit >= base) {
      *success = false;
      return 0;
    }

    val = val * base + digit;
  }

  *success = true;
  return val;
}

static uint32_t get_reg_val(const char *name, bool *success) {
  int i;

  if (name[0] == '$') {
    name ++;
  }

  if (strcmp(name, "eip") == 0) {
    *success = true;
    return cpu.eip;
  }

  for (i = 0; i < 8; i ++) {
    if (strcmp(name, regsl[i]) == 0) {
      *success = true;
      return reg_l(i);
    }
  }

  for (i = 0; i < 8; i ++) {
    if (strcmp(name, regsw[i]) == 0) {
      *success = true;
      return reg_w(i);
    }
  }

  for (i = 0; i < 8; i ++) {
    if (strcmp(name, regsb[i]) == 0) {
      *success = true;
      return reg_b(i);
    }
  }

  *success = false;
  return 0;
}

static bool check_parentheses(int p, int q) {
  int i;
  int balance = 0;

  if (p > q || tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  for (i = p; i <= q; i ++) {
    if (tokens[i].type == '(') {
      balance ++;
    }
    else if (tokens[i].type == ')') {
      balance --;
      if (balance < 0) {
        return false;
      }
      if (balance == 0 && i < q) {
        return false;
      }
    }
  }

  return balance == 0;
}

static int precedence(int type) {
  switch (type) {
    case TK_AND: return 1;
    case TK_EQ:
    case TK_NEQ: return 2;
    case '+':
    case '-': return 3;
    case '*':
    case '/': return 4;
    case TK_DEREF: return 5;
    default: return 0;
  }
}

static bool is_binary_operator(int type) {
  return type == TK_AND || type == TK_EQ || type == TK_NEQ ||
    type == '+' || type == '-' || type == '*' || type == '/';
}

static int find_dominant_op(int p, int q) {
  int i;
  int balance = 0;
  int op = -1;
  int min_prec = 0x7fffffff;

  for (i = p; i <= q; i ++) {
    int type = tokens[i].type;

    if (type == '(') {
      balance ++;
      continue;
    }

    if (type == ')') {
      balance --;
      if (balance < 0) {
        return -1;
      }
      continue;
    }

    if (balance != 0 || !is_binary_operator(type)) {
      continue;
    }

    if (precedence(type) <= min_prec) {
      min_prec = precedence(type);
      op = i;
    }
  }

  if (balance != 0) {
    return -1;
  }

  if (op != -1) {
    return op;
  }

  if (tokens[p].type == TK_DEREF) {
    return p;
  }

  return -1;
}

static uint32_t eval(int p, int q, bool *success) {
  uint32_t val1, val2;
  int op;

  if (p > q) {
    *success = false;
    return 0;
  }

  if (p == q) {
    switch (tokens[p].type) {
      case TK_DEC:
        return parse_u32(tokens[p].str, 10, success);
      case TK_HEX:
        return parse_u32(tokens[p].str, 16, success);
      case TK_REG:
        return get_reg_val(tokens[p].str, success);
      default:
        *success = false;
        return 0;
    }
  }

  if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  op = find_dominant_op(p, q);
  if (op < 0) {
    *success = false;
    return 0;
  }

  if (tokens[op].type == TK_DEREF) {
    if (op != p) {
      *success = false;
      return 0;
    }

    val2 = eval(op + 1, q, success);
    if (!*success) {
      return 0;
    }
    return vaddr_read(val2, 4);
  }

  if (op == p || op == q) {
    *success = false;
    return 0;
  }

  val1 = eval(p, op - 1, success);
  if (!*success) {
    return 0;
  }

  val2 = eval(op + 1, q, success);
  if (!*success) {
    return 0;
  }

  switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/':
      if (val2 == 0) {
        *success = false;
        return 0;
      }
      return val1 / val2;
    case TK_EQ: return val1 == val2;
    case TK_NEQ: return val1 != val2;
    case TK_AND: return val1 && val2;
    default:
      *success = false;
      return 0;
  }
}

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_DEC:
          case TK_HEX:
          case TK_REG:
            if (nr_token >= (int)(sizeof(tokens) / sizeof(tokens[0]))) {
              printf("too many tokens at position %d\n%s\n%*.s^\n", position - substr_len, e, position - substr_len, "");
              return false;
            }
            if (substr_len >= (int)sizeof(tokens[nr_token].str)) {
              printf("token too long at position %d\n%s\n%*.s^\n", position - substr_len, e, position - substr_len, "");
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token ++;
            break;
          default:
            if (nr_token >= (int)(sizeof(tokens) / sizeof(tokens[0]))) {
              printf("too many tokens at position %d\n%s\n%*.s^\n", position - substr_len, e, position - substr_len, "");
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';
            nr_token ++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

uint32_t expr(char *e, bool *success) {
  int i;

  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  if (nr_token == 0) {
    *success = false;
    return 0;
  }

  for (i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 || !is_value_token(tokens[i - 1].type))) {
      tokens[i].type = TK_DEREF;
    }
  }

  *success = true;
  return eval(0, nr_token - 1, success);
}
