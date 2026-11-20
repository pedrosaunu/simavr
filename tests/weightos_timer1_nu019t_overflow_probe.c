/* Observer-only test harness, executes real AVR code from reset. */
#include "sim_avr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
int main(int argc,char **argv){
 if(argc!=4)return 2;
 unsigned low=(unsigned)strtoul(argv[2],NULL,0);
 unsigned nops=(unsigned)strtoul(argv[3],NULL,0);
 if(low<0xf7||low>0xff||nops>8)return 2;
 uint8_t image[32768]={0};
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof(image),f);fclose(f);
 if(n<80||n>=sizeof(image)||n%2)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");
 if(!a||avr_init(a))return 2;
 a->frequency=16000000;
 avr_loadcode(a,image,(uint32_t)n,0);
 unsigned step=0,stopped=0;
 for(step=1;step<=65;step++){
  int rc=avr_run(a);
  if(rc!=cpu_Running&&rc!=cpu_Sleeping)break;
  unsigned pc=(unsigned)a->pc/2U;
  printf("NU019T_GUEST low=%u nops=%u step=%u pc=%04x cycles=%llu "
   "r19=%02x r20=%02x r21=%02x tifr1=%02x tccr1b=%02x\n",
   low,nops,step,pc,(unsigned long long)a->cycle,
   (unsigned)a->data[19],(unsigned)a->data[20],
   (unsigned)a->data[21],(unsigned)a->data[0x36],
   (unsigned)a->data[0x81]);
  if(pc>=0x39+nops && a->data[0x81]==0){
   stopped=1;break;
  }
 }
 printf("NU019T_SUMMARY low=%u nops=%u steps=%u "
   "r19_before=%02x r20_after=%02x r21_stopped=%02x "
   "TIFR1=%02x stopped=%u\n",
   low,nops,step,(unsigned)a->data[19],
   (unsigned)a->data[20],(unsigned)a->data[21],
   (unsigned)a->data[0x36],stopped);
 avr_terminate(a);
 return stopped?0:4;
}
