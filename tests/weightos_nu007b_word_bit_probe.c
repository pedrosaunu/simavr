/* NU-007B: word arithmetic and flag-bit instructions. Each case begins
 * from reset, all operands are established by executed LDI/OUT instructions. */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint16_t ldi(unsigned r,unsigned v){return 0xe000|((v&0xf0)<<4)|((r-16)<<4)|(v&15);}
static unsigned status(avr_t*a){unsigned s=0;for(unsigned k=0;k<8;k++)s|=(a->sreg[k]?1U:0U)<<k;return s;}
static int trial(avr_t*a,const char*name,unsigned x,unsigned k,unsigned f,unsigned slot,uint16_t word){
 uint16_t code[14]={ldi(16,x&255),ldi(17,x>>8),ldi(18,f),0,0,0,0,0,0,0,0,0xbf2f,word,0};
 for(unsigned i=0;i<4;i++){code[3+2*i]=ldi(24+2*i,x&255);code[4+2*i]=ldi(25+2*i,x>>8);}
 uint8_t image[sizeof code];for(unsigned i=0;i<14;i++){image[2*i]=code[i]&255;image[2*i+1]=code[i]>>8;}
 avr_reset(a);a->cycle=0;avr_loadcode(a,image,sizeof image,0);
 for(unsigned i=0;i<12;i++)if(avr_run(a)!=cpu_Running)return 3;
 if(a->pc!=24||a->cycle!=12||status(a)!=f||a->data[16]!=(x&255))return 4;
 if(avr_run(a)!=cpu_Running)return 5;
 printf("%s,%u,%u,%u,%u,%u,%u,%u",name,x,k,f,slot,a->pc/2,status(a),(unsigned)a->cycle);
 for(unsigned i=0;i<32;i++)printf(",%u",a->data[i]);putchar('\n');return 0;
}
int main(void){
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 2;a->frequency=16000000;
 unsigned v[]={0,1,15,16,63,64,255,256,32767,32768,65534,65535};
 unsigned ks[]={0,1,2,7,15,31,32,63};unsigned fs[]={0,1,2,3,0x40,0x83,0xfc,0xff};
 for(unsigned op=0;op<2;op++)for(unsigned i=0;i<12;i++)for(unsigned j=0;j<8;j++)
 for(unsigned f=0;f<8;f++)for(unsigned slot=0;slot<4;slot++){
  unsigned k=ks[j];uint16_t word=(op?0x9700:0x9600)|((k&0x30)<<2)|(slot<<4)|(k&15);
  int rc=trial(a,op?"sbiw":"adiw",v[i],k,fs[f],slot,word);if(rc)return rc;
 }
 for(unsigned op=0;op<2;op++)for(unsigned f=0;f<256;f++)for(unsigned k=0;k<8;k++){
  int rc=trial(a,op?"bclr":"bset",165,k,f,0,(op?0x9488:0x9408)|(k<<4));if(rc)return rc;
 }
 unsigned bf[]={0,0x40,0xbf,0xff};
 for(unsigned op=0;op<2;op++)for(unsigned x=0;x<256;x++)for(unsigned k=0;k<8;k++)for(unsigned f=0;f<4;f++){
  int rc=trial(a,op?"bld":"bst",x,k,bf[f],0,(op?0xf900:0xfb00)|k);if(rc)return rc;
 }
 avr_terminate(a);return 0;
}
