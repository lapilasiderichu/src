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

#include "sdb.h"

#define NR_WP 32
#define NR_EXPR 256
typedef struct watchpoint {
  int NO;
	char expr[NR_EXPR];
	word_t value;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }
  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void new_wp(char *arg,word_t value, bool *success){
	if(free_==NULL){
			printf("wp is full!");
			*success=false;
			return;
		}
	WP* wp; 
	wp=free_;
	wp->expr[0]='\0';
	for(int i=0; *(arg+i)!='\0';++i){
		wp->expr[i]=*(arg+i);
		wp->expr[i+1]='\0';
	}  
	wp->value=value;
	free_=free_->next; // 空置的头后移一个
	wp->next=NULL;
	if(head==NULL) head=wp;
	else {
					wp->next=head;    //放在使用的前面
					head=wp;
	}				 									//头指针指向最前
	printf("add a wp #%d  <%s>  : %016lx\n",wp->NO,wp->expr,wp->value);
	*success=true;
	return;
}

void free_wp(int n){
	if (head==NULL){
		printf("all watchpoint has been released.\n");			
		assert(0);
	} 
	else if (head->NO==n){
		WP* p = head;
		head=head->next;
		p->next=free_;
		free_=p;
		strcpy( free_->expr,"\n");
		free_->value=0;
		return;
	}
	else{
		for(WP* p=head;p->next!=NULL;p=p->next){
			if( (p->next)->NO==n){
				WP* q = p->next;
				p->next=q->next;
				q->next=free_;
				free_=q;
				strcpy( free_->expr,"\n");
				free_->value=0;
				return;
			}
		}
	}
}

void printf_w(){
		for(WP* p=head;p!=NULL;p=p->next){
			printf("wp#%d <%s> :%016lx\n",p->NO,p->expr,p->value);
		}
}

bool check_wp( ){
		int change_flag=0;
		for(WP* p=head;p!=NULL;p=p->next){
			bool success;
			word_t new_val = expr(p->expr,&success);
			if (new_val!=p->value){
				printf("wp#%d <%s> has  changed:%016lx==>%016lx\n",p->NO,p->expr,p->value,new_val);
				p->value=new_val;
				change_flag++;
			}			
		}
		if(change_flag>0) return true;
		else return false;
}
