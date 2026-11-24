#include "sim_avr.h"
#include "sim_interrupts.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
static void no_wall_sleep(avr_t *a, avr_cycle_count_t d){
 printf("NU008A_WAIT round=%u from=%llu requested=%llu next_due=%llu\n",a->data[0x100],(unsigned long long)a->cycle,(unsigned long long)d,(unsigned long long)(a->cycle+d));
}
static void pending(avr_irq_t *irq,uint32_t value,void *param){
 (void)irq;avr_t*a=param;
 if(value&&a->state==cpu_Sleeping)
  printf("NU008A_PENDING round=%u cycle=%llu flags=%u mask=%u\n",a->data[0x100],(unsigned long long)a->cycle,a->data[0x36],a->data[0x6f]);
}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[32768]={0};FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);
 if(!n||n>=sizeof image||(n&1))return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;a->sleep=no_wall_sleep;avr_loadcode(a,image,(uint32_t)n,0);
 for(unsigned v=11;v<=13;v++){avr_irq_t*q=avr_get_interrupt_irq(a,v);if(!q)return 3;avr_irq_register_notify(q+AVR_INT_IRQ_PENDING,pending,a);}
 uint16_t rows[500][21]={{0}};
 unsigned steps=0,entries=0,sleeps=0,errors=0,done=0,last_round=0;
 for(unsigned i=1;i<=500;i++){
  int rc=avr_run(a);if(rc!=cpu_Running&&rc!=cpu_Sleeping){errors++;break;}
  unsigned round=a->data[0x100];if(!round)continue;
  unsigned pc=a->pc/2U,sp=word(a->data,0x5d),sleep=rc==cpu_Sleeping;
  unsigned sreg=0;for(unsigned k=0;k<8;k++)sreg|=(a->sreg[k]?1U:0U)<<k;
  if(sleep&&last_round!=round){sleeps++;last_round=round;}
  if(pc==22||pc==24||pc==26){
   unsigned expected=round==1?22:round==2?24:26;
   if(pc!=expected||sp!=0x13d||sreg&128)errors++;
   entries++;
  }
  if(a->data[0x101]!=0x69||a->data[0x12b]!=0xd2)errors++;
  unsigned values[21]={steps+1,(unsigned)a->cycle,pc,sp,sreg,sleep,round,a->data[0x53],a->data[0x36],a->data[0x6f],a->data[16],a->data[17],a->data[20],a->data[21],a->data[22],a->data[23],a->data[24],a->data[25],a->data[0x13f],a->data[0x13e],a->data[0x81]};
  for(unsigned k=0;k<21;k++){if(values[k]>65535)errors++;rows[steps][k]=values[k];}
  steps++;
  if(round==3&&a->data[25]==3&&sp==0x13f){done=1;break;}
 }
 for(unsigned i=0;i<steps;i+=3){
  unsigned count=steps-i<3?steps-i:3;
  for(unsigned copy=0;copy<2;copy++){
   printf("NU008A_PACK start=%u count=%u hex=",i,count);
   for(unsigned j=i;j<i+count;j++)for(unsigned k=0;k<21;k++)
    printf("%02x%02x",rows[j][k]&255,rows[j][k]>>8);
   putchar('\n');
  }
 }
 int pass=done&&entries==3&&sleeps==3&&!errors&&a->data[20]==1&&a->data[21]==1&&a->data[22]==1&&a->data[24]==3;
 printf("NU008A_SUMMARY steps=%u entries=%u sleeps=%u done=%u errors=%u pass=%d\n",steps,entries,sleeps,done,errors,pass);
 avr_terminate(a);return pass?0:1;
}
