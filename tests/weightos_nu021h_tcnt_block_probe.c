/* NU-021H: independent simavr guest check against datasheet section 15.7.2.
 * No host writes any Timer1, AVR CPU or interrupt state. All snapshots
 * from independently executed real AVR firmware beginning at RESET.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_STEPS 160
static unsigned expected(unsigned c) {
 const unsigned x[]={4,6,0,6,4,2,4,6,4};
 return c<9?x[c]:255;
}
static int run(const char *path,unsigned testcase){
 uint8_t image[32768]={0};
 FILE *f=fopen(path,"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof(image),f);fclose(f);
 if(n<0x70||n>=sizeof(image)||n%2)return 2;
 avr_t *a=avr_make_mcu_by_name("atmega328p");
 if(!a||avr_init(a))return 3;
 a->frequency=16000000;avr_loadcode(a,image,(uint32_t)n,0);
 unsigned step=0,done=0,flags_at_write=255;
 for(unsigned i=1;i<=MAX_STEPS;i++){
  int rc=avr_run(a);
  if(rc!=cpu_Running&&rc!=cpu_Sleeping){
   fprintf(stderr,"NU021H_STOP case=%u step=%u rc=%d\n",testcase,i,rc);
   avr_terminate(a);return 4;
  }
  step=i;unsigned pc=(unsigned)a->pc/2U;
  unsigned flags=(unsigned)a->data[0x36];
  if(a->data[24]==0xa5 && a->data[25]==0x5a && flags_at_write==255)
   flags_at_write=flags;
  printf("NU021H_TRACE case=%u step=%u pc_word=%04x cycle=%llu "
    "r20=%02x r21=%02x r24=%02x r25=%02x "
    "r26=%02x r27=%02x r28=%02x r29=%02x "
    "tifr1=%02x tccr1b=%02x\n",
    testcase,step,pc,(unsigned long long)a->cycle,
    (unsigned)a->data[20],(unsigned)a->data[21],
    (unsigned)a->data[24],(unsigned)a->data[25],
    (unsigned)a->data[26],(unsigned)a->data[27],
    (unsigned)a->data[28],(unsigned)a->data[29],
    flags,(unsigned)a->data[0x81]);
  /* Stop once the CPU has completed all guest TIFR snapshots and
   * guest TCNT1 readbacks, before simavr reports cpu_Done at image end. */
  if(pc >= n/2U-3U && a->data[25]==0x5a){done=1;break;}
 }
 unsigned actual=(unsigned)a->data[29]&6;
 unsigned want=expected(testcase);
 int pass=done && a->data[25]==0x5a && actual==want;
 printf("NU021H_SUMMARY case=%u steps=%u expected_compare=%02x "
   "observed_compare=%02x guest_early=%02x guest_mid=%02x "
   "guest_late=%02x flags_at_write=%02x pass=%d\n",
   testcase,step,want,actual,(unsigned)a->data[26],
   (unsigned)a->data[27],(unsigned)a->data[28],flags_at_write,pass);
 avr_terminate(a);return pass?0:1;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;
 unsigned c=(unsigned)strtoul(argv[2],NULL,0);
 if(c>8)return 2;
 return run(argv[1],c);
}
