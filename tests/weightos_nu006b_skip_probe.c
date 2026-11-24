/* NU-006B: real skip timing and next-instruction effects, five skip
 * families, five following instructions, both decisions. */
#include "sim_avr.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
static uint16_t ldi(unsigned r,unsigned v){return 0xe000|((v&0xf0)<<4)|((r-16)<<4)|(v&15);}
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
int main(void){
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 2;a->frequency=16000000;
 uint16_t skip[]={0x1301,0xfd00,0xff00,0x9928,0x9b28};
 uint16_t next[]={0,0x9300,0x9140,0x940e,0x940c};
 uint16_t ext[]={0,0x100,0x100,0x30,0x30};
 for(unsigned op=0;op<5;op++)for(unsigned follow=0;follow<5;follow++)for(unsigned take=0;take<2;take++){
  unsigned x=op==0?0x55:0x54+(take==(op==2||op==4)),y=op==0?(take?x:x^1):0x66;
  uint16_t code[64]={ldi(16,x),ldi(17,y),0xb905,ldi(18,1),0xbf2e,ldi(18,0x3f),0xbf2d,
    ldi(19,0x69),0x9330,0x0100,0x9330,0x013f,0x9330,0x013e,ldi(18,0xa5),0xbf2f,ldi(20,0)};
  code[17]=skip[op];code[18]=next[follow];code[19]=ext[follow];
  uint8_t image[128];for(unsigned k=0;k<64;k++){image[2*k]=code[k]&255;image[2*k+1]=code[k]>>8;}
  avr_reset(a);a->cycle=0;avr_loadcode(a,image,sizeof image,0);
  for(unsigned k=0;k<14;k++)if(avr_run(a)!=cpu_Running)return 3;
  if(a->pc!=34||a->cycle!=17||word(a->data,0x5d)!=319)return 4;
  for(unsigned k=1;k<=2;k++){
   if(avr_run(a)!=cpu_Running)return 5;
   unsigned sreg=0;for(unsigned j=0;j<8;j++)sreg|=(a->sreg[j]?1U:0U)<<j;
   for(unsigned copy=0;copy<2;copy++)printf("NU006B_ROW %u,%u,%u,%u,%u,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",op,follow,take,k,a->pc/2,(unsigned long long)a->cycle,word(a->data,0x5d),sreg,a->data[16],a->data[17],a->data[18],a->data[19],a->data[20],a->data[0x100],a->data[0x13f],a->data[0x13e]);
  }
 }
 avr_terminate(a);puts("NU006B_SUMMARY cases=50 rows=100");return 0;
}
