#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
int main(int argc,char**argv){
 if(argc!=3)return 2;
 unsigned top=strtoul(argv[2],0,0);
 if(top!=0x13f&&top!=0x8ff)return 2;
 uint8_t image[32768]={0};FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof image,f);fclose(f);if(!n||n>=sizeof image||(n&1))return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
 a->frequency=16000000;avr_loadcode(a,image,(uint32_t)n,0);
 unsigned steps=0,done=0,errors=0,min_sp=65535;
 for(unsigned i=0;i<100;i++){
  if(avr_run(a)!=cpu_Running){errors++;break;}
  if(a->data[0x103]!=1)continue;
  unsigned sp=word(a->data,0x5d),pc=a->pc/2U,sreg=0;
  for(unsigned k=0;k<8;k++)sreg|=(a->sreg[k]?1U:0U)<<k;
  if(sp<min_sp)min_sp=sp;
  if(a->data[0x102]!=0xd2||a->data[top-2]!=0x69)errors++;
  printf("NU008B_ROW %u %u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",top,++steps,(unsigned long long)a->cycle,pc,sp,sreg,a->data[16],a->data[17],a->data[18],a->data[20],a->data[21],a->data[22],a->data[23],a->data[0x100],a->data[0x101],a->data[0x102],a->data[top],a->data[top-1],a->data[top-2]);
  if(a->data[23]==0xa5){done=1;break;}
 }
 int pass=done&&!errors&&min_sp==0xff&&word(a->data,0x5d)==top&&a->data[20]==1&&a->data[21]==0x5a&&a->data[22]==2;
 printf("NU008B_SUMMARY top=%u steps=%u min_sp=%u errors=%u pass=%d\n",top,steps,min_sp,errors,pass);
 avr_terminate(a);return pass?0:1;
}
