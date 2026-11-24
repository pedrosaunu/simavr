/* NU-007A: independently execute a real eight-instruction AVR fixture per case.
 * No injected GPR or status state: LDI/OUT execute from reset. */
#include "sim_avr.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
struct op {const char*name;uint16_t word;int imm;};
static const struct op ops[]={
 {"add",0x0f01,0},{"adc",0x1f01,0},{"sub",0x1b01,0},{"sbc",0x0b01,0},
 {"cp",0x1701,0},{"cpc",0x0701,0},{"and",0x2301,0},{"or",0x2b01,0},{"eor",0x2701,0},
 {"cpi",0x3000,1},{"subi",0x5000,1},{"sbci",0x4000,1},{"andi",0x7000,1},{"ori",0x6000,1},
 {"inc",0x9503,0},{"dec",0x950a,0},{"neg",0x9501,0},{"com",0x9500,0},
 {"lsr",0x9506,0},{"asr",0x9505,0},{"ror",0x9507,0},{"swap",0x9502,0},
 {"mul",0x9f01,0},{"muls",0x0201,0},{"mulsu",0x0301,0},{"fmul",0x0309,0},
 {"fmuls",0x0381,0},{"fmulsu",0x0389,0}};
static const unsigned operands[]={0,1,7,8,15,16,63,127,128,129,254,255};
static const unsigned flags[]={0,1,2,3,0x40,0x43,0x7c,0x7f};
static uint16_t ldi(unsigned r,unsigned v){return 0xe000|((v&0xf0)<<4)|((r-16)<<4)|(v&15);}
int main(void){
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 2;
 a->frequency=16000000;
 for(unsigned op=0;op<sizeof ops/sizeof *ops;op++){
  for(unsigned i=0;i<sizeof operands/sizeof *operands;i++)
  for(unsigned j=0;j<sizeof operands/sizeof *operands;j++)
  for(unsigned f=0;f<sizeof flags/sizeof *flags;f++){
   unsigned x=operands[i],y=operands[j],s=flags[f];
   uint16_t w=ops[op].word;if(ops[op].imm)w|=((y&0xf0)<<4)|(y&15);
   uint16_t code[]={ldi(19,0),0x2e03,0x2e13,ldi(16,x),ldi(17,y),ldi(18,s),0xbf2f,w,0,0}; /* OUT SREG,r18 */
   uint8_t image[sizeof code];for(unsigned k=0;k<sizeof code/2;k++){image[2*k]=code[k]&255;image[2*k+1]=code[k]>>8;}
   avr_reset(a);avr_loadcode(a,image,sizeof image,0);a->cycle=0;
   for(unsigned k=0;k<7;k++)if(avr_run(a)!=cpu_Running)return 3;
   unsigned before=0;for(unsigned k=0;k<8;k++)before|=(a->sreg[k]?1U:0U)<<k;
   if(a->pc!=14||a->data[16]!=x||a->data[17]!=y||before!=s||a->cycle!=7)return 4;
   if(avr_run(a)!=cpu_Running)return 5;
   unsigned status=0;for(unsigned k=0;k<8;k++)status|=(a->sreg[k]?1U:0U)<<k;
   printf("%s,%u,%u,%u,%u,%u,%u",ops[op].name,x,y,s,a->pc/2,status,(unsigned)a->cycle);
   for(unsigned k=0;k<32;k++)printf(",%u",a->data[k]);putchar('\n');
  }
 }
 avr_terminate(a);return 0;
}
