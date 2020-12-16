#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

uint32_t  eval(uint32_t p, uint32_t q);
bool check_parentheses(int p, int q);

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_ADD, TK_SUB, TK_MUL, TK_DIV, 
  TK_NUM, TK_LB, TK_RB
};  // 运算符号优先级内嵌在这里了!

static struct rule {
  char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  {"[0-9]+", TK_NUM},   // 十进制整数(0开头? todo)
  {"\\+", TK_ADD},         // plus
  {"\\-", TK_SUB},
  {"\\*", TK_MUL},
  {"\\/", TK_DIV},
  {"\\(", TK_LB},       // 左右括号
  {"\\)", TK_RB}
};


#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

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

static Token tokens[320] __attribute__((used)) = {};     // 只能支持解析320个token!!!
static int nr_token __attribute__((used))  = 0;

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

        switch (rules[i].token_type) {
            case TK_NOTYPE:
                break;      // 空格不记录
            case TK_NUM:
                tokens[nr_token].type = TK_NUM;
                memcpy(tokens[nr_token].str, substr_start, substr_len);     // todo: 缓冲区溢出处理
                nr_token++; break;
            default:
                tokens[nr_token++].type = rules[i].token_type; break;   // 默认处理都相同
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
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  return eval(0, nr_token-1);
}

int find_main_token(uint32_t p, uint32_t q) {
//    struct token op[32] = {};   // 所有的符号token
    uint32_t op = p;
    int lb_num = 0;
    for (int i = p; i <= q; ++i) {
        if(tokens[i].type == TK_LB){// 遇到括号就跳过, 注意括号匹配!
            lb_num++;
        } else if(tokens[i].type == TK_RB){
            lb_num--;
        } else if(tokens[i].type >= TK_ADD && tokens[i].type <= TK_DIV && lb_num == 0){ // 不在括号内
            if(tokens[i].type <= tokens[op].type)
                op = i;
        }
    }
    return op;
}

uint32_t  eval(uint32_t p, uint32_t q) {
//    return check_parentheses(p, q);
    if (p > q) {
        Log("Bad expression");
    }
    else if (p == q) {
        return strtol(tokens[p].str, NULL, 10);
    }
    else if (check_parentheses(p, q) == true) {
        return eval(p + 1, q - 1);
    }
    else {
        int op = find_main_token(p, q);      //the position of 主运算符 in the token expression;
        uint32_t val1 = eval(p, op - 1);
        uint32_t val2 = eval(op + 1, q);

        int op_type = tokens[op].type;
        switch (op_type) {
            case TK_ADD: return val1 + val2;
            case TK_SUB: return val1 - val2;
            case TK_MUL: return val1 * val2;
            case TK_DIV: return val1 / val2;
            default: assert(0);
        }
    }
    return 0;
}

bool check_parentheses(int p, int q) {
    if(tokens[p].type != TK_LB) return 0;
    if(tokens[q].type != TK_RB) return 0;
    int lb_num = 0;
    for (int i = p+1; i < q; ++i) {
        if(tokens[i].type == TK_LB) lb_num++;
        if(tokens[i].type == TK_RB){
            lb_num--;
            if(lb_num < 0) return 0;
        }
    }
    return lb_num == 0;
}
