/* Independent architectural boundary trace; external PB0 only. */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define MAX 450
#define WIDTH 41
static unsigned word(uint8_t *p,unsigned x){return p[x]+256U*p[x+1];}
int main(int argc,char **argv){
 if(argc!=2)return 2;
 uint8_t image[32768]={0},rows[MAX][WIDTH]={{0}};
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);
 if(!n||n>=sizeof image||(n&1))return 2;
 avr_t *a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;
 avr_irq_t *pin=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);
 if(!pin)return 3;
 avr_raise_irq(pin,0);avr_loadcode(a,image,(uint32_t)n,0);
 unsigned initial=word(a->data,0x5d),steps=0,maxdepth=0,entries=0,errors=0,done=0,ready=0;
 uint64_t epoch=0;int level=0;
 for(unsigned i=1;i<=MAX;i++){
  int rc=avr_run(a);if(rc!=cpu_Running&&rc!=cpu_Sleeping){errors++;break;}
  unsigned pc=a->pc/2U,sp=word(a->data,0x5d);
  if(sp>initial||initial-sp>16){errors++;break;}
  unsigned depth=initial-sp;if(depth>maxdepth)maxdepth=depth;
  if(pc==0x14){entries++;if(a->sreg[S_I])errors++;}
  uint8_t *v=rows[i-1];
  v[0]=i;v[1]=i>>8;v[2]=pc;v[3]=pc>>8;v[4]=a->cycle;v[5]=a->cycle>>8;
  for(unsigned k=0;k<12;k++)v[6+k]=a->data[16+k];
  v[18]=0;for(unsigned k=0;k<8;k++)v[18]|=(a->sreg[k]?1U:0U)<<k;
  v[19]=a->data[0x36];v[20]=a->data[0x6f];v[21]=depth;
  for(unsigned k=0;k<16;k++)v[22+k]=a->data[initial-k];
  v[38]=level;v[39]=a->data[0x86];v[40]=a->data[0x87];
  steps=i;
  if(!ready&&a->data[0x100]==0xa5){ready=1;epoch=a->cycle;}
  uint64_t age=ready?a->cycle-epoch:0;
  int desired=ready&&((age>=12&&age<20)||(age>=48&&age<56)||(age>=180&&age<188));
  if(desired!=level){avr_raise_irq(pin,desired);level=desired;
   printf("NU021M_PIN step=%u cycle=%llu level=%d\n",i,(unsigned long long)a->cycle,level);}
  if(!depth&&a->sreg[S_I]&&a->data[20]==3&&a->data[21]==0&&a->data[22]==3&&a->data[23]==3&&a->data[24]==3){
   done=1;break;
  }
 }
 int pass=done&&entries==3&&maxdepth==16&&!errors&&a->data[16]==0x5a&&a->data[17]==0xc3&&a->data[18]==0x35&&a->data[19]==0xa6;
 for(unsigned first=0;first<steps;first+=4){
  unsigned k=steps-first<4?steps-first:4;
  for(unsigned copy=0;copy<2;copy++){
   printf("NU021M_PACK first=%u rows=%u hex=",first+1,k);
   for(unsigned j=0;j<k;j++)for(unsigned b=0;b<WIDTH;b++)printf("%02x",rows[first+j][b]);
   putchar('\n');
  }
 }
 printf("NU021M_SUMMARY steps=%u max_stack=%u entries=%u done=%u errors=%u pass=%d\n",steps,maxdepth,entries,done,errors,pass);
 avr_terminate(a);return pass?0:1;
}
