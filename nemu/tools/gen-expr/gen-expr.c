#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65000];
int offset = 0;
int token_nums = 0;

uint32_t choose(uint32_t n){
    return rand() % n;
}

static inline void gen_num() {
    offset += sprintf(buf+offset, "%d", rand() % 1000 + 1); // 防止除0
}

static inline void gen(char i) {
    offset += sprintf(buf+offset, "%c", i);
}

static inline void gen_rand_op() {
    switch (choose(4)) {
        case 0: gen('+'); break;
        case 1: gen('-'); break;
        case 2: gen('*'); break;
        default: gen('/'); break;
    }
}

static inline void gen_rand_expr() {
//    if(token_nums >= 20) return;
    switch (choose(3)) {
        case 0: gen_num(); token_nums+=1; break;
        case 1: gen('('); gen_rand_expr(); gen(')'); token_nums+=2; break;
        default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); token_nums+=1; break;
    }
}

static char code_buf[65536];
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
      offset = 0;
      token_nums = 0;   // 最多320个!
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);    // buf -> code_buf

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
