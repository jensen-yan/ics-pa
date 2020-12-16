int init_monitor(int, char *[]);
void ui_mainloop(int);
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "monitor/expr.h"

void check_expr(){
    // 需要先init_regix!
    FILE *fp = fopen("/home/yanyue/workspace/ics2019/nemu/tools/gen-expr/input", "r");  // 需要根据自己目录修改
    assert(fp!=NULL);
    int line = 5;
    bool suc = true;
    while (line-- > 0) {
        char result[320] = {}, input[3200] = {};
        fscanf(fp, "%s %s", result, input);
        bool success = false;
        uint32_t ret = expr(input, &success);
        uint32_t ret_true = strtol(result, NULL, 10);
        if(ret != ret_true){
            Log("bad expr: %s %u != %u\n",input, ret, ret_true);
            suc = false;
        }
//        printf("Result: %u \n", ret);
    }
    if(suc == true) Log("all great!\n");
}

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  int is_batch_mode = init_monitor(argc, argv);

//  check_expr();

  /* Receive commands from user. */
  ui_mainloop(is_batch_mode);

  return 0;
}
