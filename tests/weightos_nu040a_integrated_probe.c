#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
static void no_sleep(avr_t*a,avr_cycle_count_t cycles){(void)a;(void)cycles;}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[32768];for(unsigned i=0;i<sizeof image;i++)image[i]=255;
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;size_t n=fread(image,1,sizeof image,f);fclose(f);if(n<0x7e00)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=1000;a->sleep=no_sleep;avr_loadcode(a,image,sizeof image,0);
 for(unsigned off=0;off<192;off+=64){
  printf("NU040A_ROM start=%u hex=",off);for(unsigned i=off;i<off+64;i++)printf("%04x",word(image,2*(i<128?i:0x3f00+i-128)));putchar('\n');
 }
 unsigned spm=0,irq=0,sleep=0,reset=0,loaded=0,returned=0;
 for(unsigned i=0;i<150;i++){
  unsigned old=a->pc;unsigned op=word(a->flash,old);int was_sleep=a->state==cpu_Sleeping;
  int rc=avr_run(a);unsigned pc=a->pc/2,mc=a->data[0x54],sp=word(a->data,0x5d);
  if(!was_sleep&&op==0x95e8)spm++;
  if(!was_sleep&&op==0x9588)sleep++;
  if(pc==65&&a->data[24]==90)loaded=1;
  if(pc==12){irq++;printf("NU040A_EVENT irq cycle=%llu sp=%u saved=%u\n",(unsigned long long)a->cycle,sp,a->data[0x8ff]+256U*a->data[0x8fe]);}
  if(pc==39&&a->data[25]==1&&sp==2303)returned=1;
  printf("NU040A_ROW %u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",i+1,(unsigned long long)a->cycle,pc,sp,a->data[0x60],mc,a->data[0x57],a->data[24],a->data[25],word(a->flash,0x80),word(a->flash,0x82),a->state,spm);
  if(mc&8){reset=1;break;}
  if(rc!=cpu_Running&&rc!=cpu_Sleeping)return 4;
 }
 unsigned pass=spm==5&&irq==1&&sleep==1&&reset&&loaded&&returned&&word(a->flash,0x80)==0xe58a&&word(a->flash,0x82)==0xcfde&&a->data[0x60]==8;
 printf("NU040A_REF_SUMMARY spm=%u irq=%u sleep=%u reset=%u loaded=%u returned=%u pass=%u\n",spm,irq,sleep,reset,loaded,returned,pass);
 avr_terminate(a);return pass?0:5;
}
