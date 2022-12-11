/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

word_t vaddr_read(vaddr_t addr ,int len);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }
//readline  把(nemu) 作为提示符 然后获取一行的输入 以EOF作为一行的结尾。
  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
	// 执行最大次数2^64-1 void cpo_exec(unit64_t n)
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
	// 返回 -1 用来结束sdb_mainloop函数
	nemu_state.state = NEMU_QUIT;
  return -1;
}


static int cmd_help(char *args);
//
static int cmd_si_N(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);
//
static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
	
  /* TODO: Add more commands */
	{"si", "用命令'si N'来执行N步程序，缺省N时默认执行1步:", cmd_si_N},
	{"info","打印寄存器信息'info r'或者监视点信息'info w' ",cmd_info},
	{"x", "扫描内存",cmd_x},
	{"p","表达式求值",cmd_p},
	{"w","设置监视点",cmd_w},
	{"d"," 删除监视点",cmd_d},
};

static int cmd_si_N(char *args){
	int N=0;
	char *arg = strtok(NULL," ");
	char *arg2 = strtok(NULL," ");
	if (arg2!=NULL) {
		printf("too many arguments\n");
		return 0;
	}
	//缺省N时默认执行一次
	if (arg == NULL) {
		cpu_exec(1);
		return 0;
	}
	//考虑N和错误处理
	else {
		for(int i = 0; arg[i]!=0; i++) {	
			if (arg[i]>=48 && arg[i]<=57){
				N=N*10+arg[i]-48;
			}
			else {
				printf("not a Natural number: %s\n",arg);
				return 0;
			}
		}
		cpu_exec(N);
		return 0;
	}
}

static int cmd_info(char *args) {
	bool right_reg=true;
	word_t reg_value;
	char *arg = strtok(NULL," ");
	char *arg2 = strtok(NULL," ");
	if (arg2!=NULL) {
		printf("too many arguments\n");
		return 0;
	}

	if (arg==NULL) {
		printf("need \"r or reg_name \" to dispaly register or \"w\" to dispaly watchpoint \n");
		return 0;
	}
	
	if (strcmp(arg,"r")==0){
		isa_reg_display();
	}
	else if (strcmp(arg, "w")==0)
	{
		printf_w();	
	}
	else {
		reg_value=isa_reg_str2val(arg,&right_reg);
		if(right_reg) { printf("%s:0x%016lx\n",arg,reg_value);}
		else { printf("wrong register!\n");}
	}
	return 0;
}

static int cmd_x(char *args)
{
	uint32_t mem=0;
	int N = 0;
	word_t addr = 0;
	bool expr_success;
	char *arg = strtok(NULL," ");
	char *arg2 = strtok(NULL, "\n");
	if (arg==NULL) {
		printf("缺少起始地址和N\n");
		return 0;
	}
	else {
		for(int i = 0; arg[i]!=0; i++) {
				if (arg[i]>=48 && arg[i]<=57) {
					N=N*10+arg[i]-48;
				}
				else {
					printf("wrong N\n");
					return 0;
				}
		}
		if(arg2!=NULL) {
			//printf("%s\n",arg2);
			addr = expr(arg2,&expr_success);
			for (int i =0;i<N;i++) {
				mem=vaddr_read(addr+4*i,4);
				printf("0x%016lx:0x%08x\n",addr+4*i,mem);
			}
			//printf("expr_success=%d",expr_success);
		  //printf("%016lx\n",addr);
		}
		else { printf("need addr\n");}
		return 0;
	}
}


static int cmd_p(char *args) {
	char *arg = strtok(NULL,"\n");
	bool success;
	word_t result = 0;
	result = expr(arg,&success);
	if (success){
		printf("result=0x%016lx\n",result);
	}
	else {
		printf("wrong expression\n");
	}
	return 0;
}


static int cmd_w(char * args){
	bool success;
	word_t value =0;
	value= expr(args,&success);
	//printf("value is %016lx",value);
	if (!success) printf("wrong watchpoint!\n");
	else new_wp(args, value, &success);
	//printf("%s",arg);
	//assert(success);
	return 0;				
}

static int cmd_d(char *args){
	
	char *arg = strtok(NULL,"\n");
	int 	n = atoi(arg);
	free_wp(n);
	return 0;
}

#define NR_CMD ARRLEN(cmd_table)

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

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
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

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }			//判断是否需要推出sdb_mainloop函数
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
