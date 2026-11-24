/* Exact byte-delta encoded architectural trace after firmware sets SP.
 * PB0 is the only input. No external writes to guest registers or SRAM. */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define MAX 800
#define WIDTH 50
static unsigned word(uint8_t *p,unsigned x){return p[x]+256U*p[x+1];}
static void putword(uint8_t *p,unsigned x,unsigned v){p[x]=v;p[x+1]=v>>8;}
static int waveform(uint64_t age){
 for(unsigned k=0;k<3;k++){
  unsigned start=12+180*k;
  if((age>=start&&age<start+8)||(age>=start+36&&age<start+44))return 1;
 }
 return 0;
}
int main(int argc,char **argv){
 if(argc!=2)return 2;
 uint8_t image[32768]={0},rows[MAX][WIDTH]={{0}};
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);
 if(!n||n>=sizeof image||(n&1))return 2;
 avr_t *a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;
 avr_irq_t *pin=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);if(!pin)return 3;
 avr_raise_irq(pin,0);avr_loadcode(a,image,(uint32_t)n,0);
 unsigned steps=0,maxdepth=0,entries=0,errors=0,done=0,ready=0,peak_rounds=0,prior_depth=0,returns=0;
 uint64_t epoch=0;int level=0;
 for(unsigned i=1;i<=MAX;i++){
  int rc=avr_run(a);if(rc!=cpu_Running&&rc!=cpu_Sleeping){errors++;break;}
  if(!ready&&a->data[0x100]==0xa5){ready=1;epoch=a->cycle;}
  if(!ready)continue;
  unsigned pc=a->pc/2U,sp=word(a->data,0x5d);
  if(sp>0x13f||0x13f-sp>20){errors++;break;}
  unsigned depth=0x13f-sp;
  if(depth>maxdepth)maxdepth=depth;
  if(depth==20&&prior_depth!=20)peak_rounds++;
  if(!depth&&prior_depth)returns++;
  prior_depth=depth;
  if(pc==0x14){entries++;if(a->sreg[S_I])errors++;}
  if(a->data[0x101]!=0x69||a->data[0x12b]!=0xd2)errors++;
  uint8_t *v=rows[steps];
  putword(v,0,steps+1);putword(v,2,pc);putword(v,4,a->cycle);putword(v,6,sp);
  for(unsigned k=0;k<12;k++)v[8+k]=a->data[16+k];
  v[20]=a->data[30];v[21]=a->data[31];
  for(unsigned k=0;k<8;k++)v[22]|=(a->sreg[k]?1U:0U)<<k;
  v[23]=a->data[0x36];v[24]=a->data[0x6f];v[25]=level;
  v[26]=a->data[0x86];v[27]=a->data[0x87];
  for(unsigned k=0;k<20;k++)v[28+k]=a->data[0x13f-k];
  v[48]=a->data[0x101];v[49]=a->data[0x12b];
  steps++;
  int desired=waveform(a->cycle-epoch);
  if(desired!=level){avr_raise_irq(pin,desired);level=desired;
   printf("NU021N_PIN step=%u cycle=%llu level=%d\n",steps,(unsigned long long)a->cycle,level);}
  if(!depth&&a->sreg[S_I]&&a->data[20]==6&&a->data[21]==0&&a->data[22]==6&&a->data[23]==6&&a->data[24]==6){
   done=1;break;
  }
 }
 int pass=done&&entries==6&&maxdepth==20&&peak_rounds==3&&returns==3&&!errors&&
 a->data[16]==0x5a&&a->data[17]==0xc3&&a->data[18]==0x35&&a->data[19]==0xa6;
 /* Lossless delta stream: 8-byte little-endian changed-byte bitmap then
  * new bytes in ascending offset order. First row relative to all zeros. */
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
   printf("NU021N_PACK offset=%u bytes=%u hex=",first,k);
   for(unsigned j=0;j<k;j++)printf("%02x",packed[first+j]);
   putchar('\n');
  }
 }
 printf("NU021N_SUMMARY steps=%u packed=%u max_stack=%u peaks=%u entries=%u outer_returns=%u done=%u errors=%u pass=%d\n",steps,bytes,maxdepth,peak_rounds,entries,returns,done,errors,pass);
 avr_terminate(a);return pass?0:1;
}
