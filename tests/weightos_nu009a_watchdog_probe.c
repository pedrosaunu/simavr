/* Real AVR WDT witnesses. CPU=1000 Hz, simulated watchdog oscillator
 * 128 kHz, chosen solely to shorten the witness, not physical board timing.
 * No host mutation of guest registers, flags or SRAM. */
#include "sim_avr.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
static unsigned word(uint8_t*p,unsigned i){return p[i]+256U*p[i+1];}
int main(void){
 for(unsigned mode=0;mode<3;mode++){
  avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 2;a->frequency=1000;
  uint16_t code[128]={0};code[0]=0xc037;code[12]=0xc063; /* rjmp 112 */
  unsigned i=56;
  code[i++]=0x94f8; /* cli */
  code[i++]=0xe001;code[i++]=0xbf0e; /* SPH=1 */
  code[i++]=0xe30f;code[i++]=0xbf0d; /* SPL=0x3f */
  code[i++]=0xe609;code[i++]=0x9300;code[i++]=0x100;
  code[i++]=0xe000|((mode==0?0x40:mode==1?0x08:0x48)&15)|(((mode==0?0x40:mode==1?0x08:0x48)&0xf0)<<4);
  code[i++]=0x9300;code[i++]=0x60;
  code[i++]=0x95a8; /* WDR */
  for(unsigned j=0;j<8;j++)code[i++]=0;
  code[i++]=0x95a8; /* WDR postpones expiry */
  code[i++]=0x9478;code[i++]=0;code[i++]=0xcffe; /* loop NOP/rjmp */
  code[112]=0x9543;code[113]=0x9518; /* inc r20, reti */
  uint8_t image[256];for(unsigned j=0;j<128;j++){image[2*j]=code[j]&255;image[2*j+1]=code[j]>>8;}
  avr_loadcode(a,image,sizeof image,0);
  printf("NU009A_WORDS mode=%u hex=",mode);for(unsigned j=0;j<128;j++)printf("%04x",code[j]);putchar('\n');
  unsigned entries=0,reset=0,done=0;
  for(unsigned j=0;j<120;j++){
   int rc=avr_run(a);if(rc!=cpu_Running)return 3;
   unsigned pc=a->pc/2,sp=word(a->data,0x5d),csr=a->data[0x60],mc=a->data[0x54];
   if(pc==12){entries++;printf("NU009A_IRQ mode=%u cycle=%llu sp=%u csr=%u saved=%u\n",mode,(unsigned long long)a->cycle,sp,csr,a->data[0x13f]+256U*a->data[0x13e]);}
   if(mc&8){reset=1;printf("NU009A_RESET mode=%u cycle=%llu pc=%u csr=%u mcusr=%u ram=%u\n",mode,(unsigned long long)a->cycle,pc,csr,mc,a->data[0x100]);done=1;break;}
   if(mode==0&&a->data[20]==2&&sp==319){done=1;break;}
   if(a->data[0x100]==0x69)printf("NU009A_ROW %u,%llu,%u,%u,%u,%u,%u,%u\n",mode,(unsigned long long)a->cycle,pc,sp,csr,mc,a->data[20],a->data[0x100]);
  }
  printf("NU009A_CASE mode=%u entries=%u reset=%u done=%u\n",mode,entries,reset,done);
  if(!done||entries!=(mode==0?2:mode==1?0:1)||reset!=(mode!=0))return 4;
  avr_terminate(a);
 }
 puts("NU009A_REFERENCE_PASS");return 0;
}
