/* Lossless absolute-stack trace, 3 rounds. Only PB0 driven. */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX 1800
#define WIDTH 52
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
static void putword(uint8_t*p,unsigned i,unsigned v){p[i]=v;p[i+1]=v>>8;}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 uint8_t image[32768]={0},rows[MAX][WIDTH]={{0}};
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);
 if(!n||n>=sizeof image||(n&1))return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;
 avr_irq_t*pin=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);if(!pin)return 3;
 avr_raise_irq(pin,0);avr_loadcode(a,image,(uint32_t)n,0);
 unsigned steps=0,round=0,entries=0,errors=0,done=0,maxdepth=0;
 unsigned sequence[18]={0},rounds[18]={0},depths[18]={0};
 unsigned pending[4]={0},captures_during_stack=0;
 uint64_t epoch=0;int level=0;
 for(unsigned i=1;i<=MAX;i++){
  int rc=avr_run(a);if(rc!=cpu_Running&&rc!=cpu_Sleeping){errors++;break;}
  unsigned marker=a->data[0x100];
  if(marker!=round){if(marker!=round+1||marker>3){errors++;break;}round=marker;epoch=a->cycle;}
  if(!round)continue;
  unsigned pc=a->pc/2U,sp=word(a->data,0x5d);
  if(sp>0x13f||0x13f-sp>20){errors++;break;}
  unsigned depth=0x13f-sp;
  if(depth>maxdepth)maxdepth=depth;
  if(!depth&&a->data[0x6f]==0x20&&(a->data[0x36]&0x27)==0x27)pending[round]=1;
  if(pc>=0x14&&pc<=0x1a&&!(pc&1)){
   if(entries>=18||a->sreg[S_I]){errors++;break;}
   sequence[entries]=pc;rounds[entries]=round;depths[entries]=depth;entries++;
  }
  if(a->data[0x101]!=0x69||a->data[0x12b]!=0xd2)errors++;
  uint8_t*v=rows[steps];
  putword(v,0,steps+1);putword(v,2,pc);putword(v,4,a->cycle);putword(v,6,sp);
  for(unsigned k=0;k<14;k++)v[8+k]=a->data[16+k];
  for(unsigned k=0;k<8;k++)v[22]|=(a->sreg[k]?1U:0U)<<k;
  v[23]=a->data[0x36];v[24]=a->data[0x6f];v[25]=level;
  putword(v,26,word(a->data,0x86));
  for(unsigned k=0;k<20;k++)v[28+k]=a->data[0x13f-k];
  v[48]=a->data[0x101];v[49]=a->data[0x12b];v[50]=marker;v[51]=a->data[0x81];
  steps++;
  uint64_t age=a->cycle-epoch;
  int desired=(age>=8&&age<17)||(age>=160&&age<169);
  if(desired!=level){
   if(desired&&age>=160&&depth>0)captures_during_stack++;
   avr_raise_irq(pin,desired);level=desired;
   printf("NU021O_PIN step=%u cycle=%llu level=%d\n",steps,(unsigned long long)a->cycle,level);
  }
  if(round==3&&!depth&&a->sreg[S_I]&&a->data[20]==6&&a->data[21]==3&&a->data[22]==3&&a->data[23]==3&&a->data[24]==3&&a->data[25]==6){
   done=1;break;
  }
 }
 unsigned expected[15]={20,22,24,20,26,20,20,22,24,26,20,22,24,20,26};
 for(unsigned i=0;i<entries;i++){
  printf("NU021O_IRQ index=%u round=%u pc=%u depth=%u\n",i+1,rounds[i],sequence[i],depths[i]);
  if(i>=15||sequence[i]!=expected[i]||rounds[i]!=i/5+1)errors++;
 }
 int pass=done&&entries==15&&pending[1]&&pending[2]&&pending[3]&&captures_during_stack==3&&!errors;
 uint8_t packed[MAX*(WIDTH+8)]={0},prev[WIDTH]={0};unsigned bytes=0;
 for(unsigned i=0;i<steps;i++){
  uint64_t mask=0;
  for(unsigned j=0;j<WIDTH;j++)if(rows[i][j]!=prev[j])mask|=UINT64_C(1)<<j;
  for(unsigned j=0;j<8;j++)packed[bytes++]=(uint8_t)(mask>>(8*j));
  for(unsigned j=0;j<WIDTH;j++)if(mask&(UINT64_C(1)<<j)){packed[bytes++]=rows[i][j];prev[j]=rows[i][j];}
 }
 for(unsigned first=0;first<bytes;first+=180){
  unsigned k=bytes-first<180?bytes-first:180;
  for(unsigned copy=0;copy<2;copy++){
   printf("NU021O_PACK offset=%u bytes=%u hex=",first,k);
   for(unsigned j=0;j<k;j++)printf("%02x",packed[first+j]);
   putchar('\n');
  }
 }
 printf("NU021O_SUMMARY steps=%u packed=%u max_stack=%u entries=%u pending_rounds=%u captures_during_stack=%u done=%u errors=%u pass=%d\n",steps,bytes,maxdepth,entries,pending[1]+pending[2]+pending[3],captures_during_stack,done,errors,pass);
 avr_terminate(a);return pass?0:1;
}
