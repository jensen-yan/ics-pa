#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
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

static int cmd_help(char *args);

static int cmd_si(char *args){
    uint64_t n = strtol(args,NULL, 10);
    cpu_exec(n);
    return 0;
}

// info r / info w
static int cmd_info(char *args){
    if(strcmp(args, "r") == 0){
        isa_reg_display();
    } else if(strcmp(args, "w") == 0){
        // todo
    } else{
        Log("Invalid cmd! Use 'info r' or 'info w'!");
    }
    return 0;
}

// p (1*2+3-2)+3
static int cmd_p(char *args){
    bool success = false;
    uint32_t ret = expr(args, &success);
    printf("Result: %u \n", ret);
    return 0;
}

// x 4 0x100000
static int cmd_x(char *args){
    char* arg1 = strtok(args, " ");
    char* arg2 = arg1 + strlen(arg1) + 1;   // todo: 只支持0x100, 还要支持eax的值这样
    long n = strtol(arg1, NULL, 10);
    uint32_t offset = strtol(arg2+2, NULL, 16); // str to hex num.  eg: 0x10 -> 16
    for (int i = 0; i < n; ++i) {
        uint32_t begin = offset + i*4;
        printf("Addr: " FMT_WORD "   ", begin);
        printf("Data: 0x%02x%02x%02x%02x\n", pmem[begin+3], pmem[begin+2], pmem[begin+1], pmem[begin]);    // 32位空间, 分成4个存放
    }

    return 0;
}

static int cmd_w(char *args){
    return 0;
}

static int cmd_d(char *args){
    return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "exec N steps", cmd_si},
  { "info", "print regs or watchpoint", cmd_info},
  { "p", "print expr", cmd_p},
  { "x", "scan expr ~ expr + n memory", cmd_x},
  { "w", "set the watchpoint", cmd_w},
  { "d", "delete the watchpoint", cmd_d}

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
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

  for (char *str; (str = rl_gets()) != NULL; ) {
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
