#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
static void no_wall_sleep(avr_t *a, avr_cycle_count_t d){(void)a;(void)d;}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[32768]={0};FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);
 if(!n||n>=sizeof image||(n&1))return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;a->sleep=no_wall_sleep;avr_loadcode(a,image,(uint32_t)n,0);
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
  printf("NU008A_ROW %u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
    ++steps,(unsigned long long)a->cycle,pc,sp,sreg,sleep,round,a->data[0x53],a->data[0x36],a->data[0x6f],a->data[16],a->data[17],a->data[20],a->data[21],a->data[22],a->data[23],a->data[24],a->data[25],a->data[0x13f],a->data[0x13e],a->data[0x81]);
  if(round==3&&a->data[25]==3&&sp==0x13f){done=1;break;}
 }
 int pass=done&&entries==3&&sleeps==3&&!errors&&a->data[20]==1&&a->data[21]==1&&a->data[22]==1&&a->data[24]==3;
 printf("NU008A_SUMMARY steps=%u entries=%u sleeps=%u done=%u errors=%u pass=%d\n",steps,entries,sleeps,done,errors,pass);
 avr_terminate(a);return pass?0:1;
}
