/* Independent simulator read-only probe. No guest register injection. */
#include "sim_avr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MAX_STEPS 64
int main(int argc,char **argv){
 if(argc!=4)return 2;
 unsigned style=(unsigned)strtoul(argv[2],NULL,0);
 unsigned running=(unsigned)strtoul(argv[3],NULL,0);
 if(style>6||running>1)return 2;
 uint8_t image[32768]={0};
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof(image),f);fclose(f);
 if(n<70||n>=sizeof(image)||(n&1))return 2;
 avr_t *avr=avr_make_mcu_by_name("atmega328p");
 if(!avr||avr_init(avr))return 2;
 avr->frequency=16000000;
 avr_loadcode(avr,image,(uint32_t)n,0);
 unsigned observed=0,done=0,matched=0;
 for(unsigned step=1;step<=MAX_STEPS;step++){
  int rc=avr_run(avr);
  if(rc!=cpu_Running&&rc!=cpu_Sleeping)break;
  unsigned pc=(unsigned)avr->pc/2U;
  unsigned count=(unsigned)avr->data[0x84]|((unsigned)avr->data[0x85]<<8);
  unsigned flag=(unsigned)avr->data[0x36];
  unsigned ctrl=(unsigned)avr->data[0x81];
  printf("NU020_BOUNDARY style=%u running=%u step=%u pc=%04x cycles=%llu "
   "r19=%02x r20=%02x r21=%02x r22=%02x r23=%02x "
   "r24=%02x r25=%02x tifr1=%02x tccr1b=%02x tcnt1=%04x\n",
   style,running,step,pc,(unsigned long long)avr->cycle,
   (unsigned)avr->data[19],(unsigned)avr->data[20],
   (unsigned)avr->data[21],(unsigned)avr->data[22],
   (unsigned)avr->data[23],(unsigned)avr->data[24],
   (unsigned)avr->data[25],flag,ctrl,count);
  observed++;
  if(avr->data[19]==0x5a){
   done=1;
   const unsigned first=(unsigned)avr->data[21];
   const unsigned last=(unsigned)avr->data[23];
   const unsigned desired=style==1?0:style==2?2:4;
   const unsigned final=style==2?2:0;
   const unsigned guest_counter = (unsigned)avr->data[24] |
                                 ((unsigned)avr->data[25] << 8);
   matched=(avr->data[20]==6&&first==desired&&avr->data[22]==desired&&
            last==final&&flag==final&&(ctrl&7U)==(running?1U:0U)&&
            guest_counter>0);
   break;
  }
 }
 printf("NU020_SUMMARY style=%u running=%u steps=%u done=%u "
  "r20=%02x r21=%02x r22=%02x r23=%02x r24=%02x r25=%02x "
  "tifr1=%02x tccr1b=%02x guest_counter=%04x pass=%u\n",
  style,running,observed,done,
  (unsigned)avr->data[20],(unsigned)avr->data[21],
  (unsigned)avr->data[22],(unsigned)avr->data[23],
  (unsigned)avr->data[24],(unsigned)avr->data[25],
  (unsigned)avr->data[0x36],(unsigned)avr->data[0x81],
  (unsigned)avr->data[24]|((unsigned)avr->data[25]<<8),matched);
 avr_terminate(avr);return matched?0:5;
}
