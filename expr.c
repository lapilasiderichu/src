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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
//word_t isa_reg_str2val(const char *s, bool *success);
//
word_t vaddr_read(vaddr_t addr ,int len);
enum {
  TK_NOTYPE = 256, TK_EQ, 

  /* TODO: Add more token types */
	TK_NUM,
	TK_HEXNUM,
	TK_REG,
	TK_DEREF,
	TK_MINUS,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
	//
	{"^0x[0-9a-f]+",TK_HEXNUM},
	{"^\\$[a-z0-9]+",TK_REG},
	{"[0-9]+",TK_NUM},
	{"-", '-'},					// subtract
	{"\\*",'*'},					// multiply
	{"/",'/'},					// divide
	{"\\(",'('},
	{"\\)",')'},
};

#define NR_REGEX ARRLEN(rules)

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

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;
//
static word_t eval(int p, int q,bool *success);
//				
static void record(int i, int substr_len, char *substr_start){
	tokens[nr_token].type=rules[i].token_type;
											for(int j = 0; j < substr_len;j++) 
															tokens[nr_token].str[j]=*(substr_start+j);
											nr_token++;
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
//匹配到数字就记录下类型和数字大小,并将token长度加一
        switch (rules[i].token_type) {
					case TK_NUM:record(i,substr_len,substr_start);
											break;

					case TK_NOTYPE:break;

					case TK_REG:record(i,substr_len,substr_start); 
											break;

					case TK_HEXNUM:record(i,substr_len,substr_start);
												 break;

          default: tokens[nr_token].type=rules[i].token_type;
									 nr_token++;
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


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;

    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
	for (int i = 0 ; i < nr_token-1;i++){
		if(tokens[i].type=='*'&&(i==0||tokens[i-1].type=='+'||tokens[i-1].type=='-'||tokens[i-1].type=='*'||tokens[i-1].type=='/'))
						tokens[i].type=TK_DEREF;
	}



	//计算成功返回success
	word_t result;
	*success =true;
	//printf("nr_token=%d\n",nr_token);	
	result = eval(0,nr_token-1,success);
  return result;

}
//  检测括号是否匹配，和会不会先出现右括号，若先出现右括号或者不匹配则返回false
static bool parentheses(int p,int q ){
	int flag=0;
	for (int i = p;i<=q && flag>=0;i++){
		if(tokens[i].type=='(') flag++;
		else if (tokens[i].type==')') flag--;
	}
	if (flag!=0) return false;
	else return true;
}

//去掉最外层的括号后判断里面的括号是否匹配，最外层没括号或者里面不匹配则返回false
static bool check_parentheses(int p,int q) {
	if (tokens[p].type=='(' && tokens[q].type==')')
	{
		return parentheses(p+1,q-1);
	}
	else {
		return false;
	}
}

static word_t eval(int p,int q,bool *success) {
	if (p > q) {
	/* Bad expression */
	//	printf("Bad expression\n");
		*success=false;
		return -1;
	}
	else if (p == q) {
	/* Single token.
	* For now this token should be a number.
	* Return the value of the number.
	 */
/** 处理记录的字符串返回表示的数字 利用strtol函数处理字符串按照不同的进制读取字符串*/
	word_t num = 0;
	switch (tokens[p].type) {
					case TK_NUM:
						num=strtol(tokens[p].str,0,10);//10进制
						break;
					case TK_HEXNUM:
						num=strtol(tokens[p].str,0,16);//16进制
						break;
					case TK_REG:
						bool success_reg=false;
						num=isa_reg_str2val(tokens[p].str+1,&success_reg);
						if(!success_reg){
										printf("%s is not a reg\n",(tokens[p].str+1));
										*success=false;
						}
						break;			
					default: *success=false;
						return -1;	
		}
		memset(tokens[p].str,0,sizeof(tokens[p].str));
//  取出数字后清空记录的数字，否则会影响下一次的求值 		
	  return num;
	}
	else if (check_parentheses(p, q) == true) {
 /* The expression is surrounded by a matched pair of parentheses.
 * If that is the case, just throw away the parentheses.
*/
		//printf("check=%d\n******",check_parentheses(p,q));			
		return eval(p + 1, q - 1,success);
	}
	else {
//   找到主运算符，先判断是否是乘除号，若是则判断其左边是否括号匹配，匹配则在括号外
		int op = p;   //op 指示主运算符位置
		for	(int i = p ; i<=q; i++){
			if(tokens[i].type=='*'||tokens[i].type=='/'){
				if(parentheses(p,i))
					op=i;
			}
		}
//  同乘除判断加减，优先级后判断，若有则覆盖乘除作为主运算符		
		for	(int i = p ; i<=q; i++){
			if(tokens[i].type=='+'||tokens[i].type=='-'){
				if(parentheses(p,i))
					op=i;
			}
		}
// 没有找到运算符且p<q，判断是否是指针解用
		if (op==p){
			if(tokens[p].type==TK_DEREF) {
				word_t addr = eval(p+1,q,success);
				return vaddr_read(addr,4);	
			}		
			else {
				*success=false;
				return -1;
			}
		}

		word_t val1 = eval(p, op - 1,success);
		word_t val2 = eval(op + 1, q,success);

		switch (tokens[op].type) {
			case '+': return val1 + val2;
			case '-': return val1 - val2;
			case '*': return val1 * val2;
			case '/': if (val2==0){ printf("division 0! \n");assert(0) ;} return val1 / val2;
			default: success = false; return -1;
		}
	}
}

