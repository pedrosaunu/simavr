/* NU-019C external-pin, real AVR guest-instruction reference.
 * Stimulus PB0 ONLY at guest instruction boundary after its arming marker.
 * This does not emulate capture, inject a flag, alter CPU registers,
 * or prove physical synchronous ICF1/W1C silicon-edge priority.
 */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
enum { CODE_MAX=32768 };
int main(int argc,char**argv){
 if(argc!=4)return 2;
 unsigned nops=(unsigned)strtoul(argv[2],NULL,10);
 int edge_delay=atoi(argv[3]); /* -1 is negative no-external-edge control */
 if(nops>5||edge_delay<-1||edge_delay>4)return 2;
 uint8_t bytes[CODE_MAX]={0};
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t len=fread(bytes,1,sizeof(bytes),f);int err=ferror(f);fclose(f);
 if(err||len<60||len>=sizeof(bytes)||len%2)return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");
 if(!a||avr_init(a))return 3;
 a->frequency=16000000;
 avr_irq_t*pb0=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);
 if(!pb0){avr_terminate(a);return 4;}
 avr_raise_irq(pb0,0);
 avr_loadcode(a,bytes,(uint32_t)len,0);
 unsigned steps=0,ready=0,done=0,injected=0;
 uint64_t ready_cycle=0,stim_cycle=0;
 for(unsigned i=0;i<90;i++){
  int rc=avr_run(a);
  if(rc!=cpu_Running&&rc!=cpu_Sleeping&&rc!=cpu_Done){
   fprintf(stderr,"NU019C_GUEST_HALTED nops=%u delay=%d rc=%d step=%u\n",
    nops,edge_delay,rc,i);avr_terminate(a);return 5;
  }
  steps=i+1;
  if(!ready && a->data[0x100]==0xa5){ready=1;ready_cycle=a->cycle;}
  if(ready && !injected && edge_delay>=0 &&
     a->cycle >= ready_cycle+(uint64_t)edge_delay){
    avr_raise_irq(pb0,1);injected=1;stim_cycle=a->cycle;
    printf("NU019C_EXTERNAL_PIN nops=%u delay=%d cycle=%llu pinb0=1\n",
       nops,edge_delay,(unsigned long long)stim_cycle);
  }
  printf("NU019C_REF_GUEST nops=%u delay=%d step=%u pc=%04x cycles=%llu "
    "r19=%02x r20=%02x r21=%02x r22=%02x r23=%02x "
    "tifr1=%02x tccr1b=%02x ready=%u injected=%u\n",
    nops,edge_delay,steps,(unsigned)(a->pc/2U),
    (unsigned long long)a->cycle,(unsigned)a->data[19],
    (unsigned)a->data[20],(unsigned)a->data[21],
    (unsigned)a->data[22],(unsigned)a->data[23],
    (unsigned)a->data[0x36],(unsigned)a->data[0x81],ready,injected);
  if(a->data[0x101]==0x5a){done=1;break;}
 }
 printf("NU019C_REF_SUMMARY nops=%u delay=%d steps=%u done=%u "
   "injected=%u ready_cycle=%llu injection_cycle=%llu "
   "r19=%02x r20=%02x r21=%02x r22=%02x r23=%02x tifr1=%02x\n",
   nops,edge_delay,steps,done,injected,
   (unsigned long long)ready_cycle,(unsigned long long)stim_cycle,
   (unsigned)a->data[19],(unsigned)a->data[20],(unsigned)a->data[21],
   (unsigned)a->data[22],(unsigned)a->data[23],
   (unsigned)a->data[0x36]);
 avr_terminate(a);
 if(!ready||!done||((edge_delay>=0)!=(injected!=0)))return 6;
 return 0;
}
