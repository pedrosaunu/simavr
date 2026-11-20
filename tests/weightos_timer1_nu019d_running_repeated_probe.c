/* NU-019D: independently run AVR binary, drive only PB0 pin externally.
 * Six named pin schedules. Every PINB level is logged at each instruction
 * boundary for lossless neural-input replay, never guest opcode simulation.
 */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { MAX_CODE=32768 };
enum { NO_EDGE=0, SINGLE=1, SPLIT=2, OVERLAP=3, LATE=4, STRESS=5 };
static const char *names[]={"none","single","split","overlap","late","stress"};
static int requested_level(int mode,int age){
  if(age<0)return 0;
  switch(mode){
  case NO_EDGE:return 0;
  case SINGLE:return 1;
  case SPLIT:return age<10?1:(age<12?0:1);
  case OVERLAP:return age<3?1:(age<5?0:1);
  case LATE:return age<10?1:(age<17?0:1);
  case STRESS:return age<3?1:(age<5?0:(age<10?1:(age<12?0:1)));
  default:return -1;
  }
}
int main(int argc,char **argv){
 /* simavr emits timer callbacks to stderr. Flush each complete guest
  * record on newline before the next avr_run() to prevent stdio block
  * buffering from interleaving the independently observed log lines. */
 if(setvbuf(stdout,NULL,_IOLBF,0))return 2;
 if(argc!=4)return 2;
 int mode=atoi(argv[2]);int pad=atoi(argv[3]);
 if(mode<0||mode>5||pad<0||pad>4||(pad%2))return 2;
 uint8_t image[MAX_CODE]={0};
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof(image),f);int err=ferror(f);fclose(f);
 if(err||n<85||n>=sizeof(image)||n%2)return 2;
 avr_t *a=avr_make_mcu_by_name("atmega328p");
 if(!a||avr_init(a))return 3;
 a->frequency=16000000;
 avr_irq_t *pb0=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);
 if(!pb0){avr_terminate(a);return 4;}
 avr_raise_irq(pb0,0);
 avr_loadcode(a,image,(uint32_t)n,0);
 int ready=0,ready_step=0,level=0,transitions=0,done=0,steps=0;
 for(int step=1;step<=85;step++){
   int rc=avr_run(a);
   if(rc!=cpu_Running&&rc!=cpu_Sleeping&&rc!=cpu_Done){
     fprintf(stderr,"NU019D_BAD_GUEST m=%d pad=%d step=%d rc=%d\n",mode,pad,step,rc);
     avr_terminate(a);return 5;
   }
   steps=step;
   if(!ready && a->data[0x100]==0xa5){ready=1;ready_step=step;}
   int age=ready?step-ready_step:-1;
   int desired=requested_level(mode,age);
   if(desired!=level){
     avr_raise_irq(pb0, (uint32_t)desired);
     level=desired;transitions++;
     printf("NU019D_PIN mode=%s pad=%d step=%d age=%d cycle=%llu level=%d\n",
       names[mode],pad,step,age,(unsigned long long)a->cycle,level);
   }
   printf("NU019D_GUEST mode=%s pad=%d step=%d pc=%04x cycles=%llu "
     "r18=%02x r19=%02x r20=%02x r21=%02x r22=%02x r23=%02x "
     "r24=%02x r25=%02x tifr1=%02x tccr1b=%02x level=%d ready=%d\n",
     names[mode],pad,step,(unsigned)(a->pc/2U),
     (unsigned long long)a->cycle,
     (unsigned)a->data[18],(unsigned)a->data[19],(unsigned)a->data[20],
     (unsigned)a->data[21],(unsigned)a->data[22],(unsigned)a->data[23],
     (unsigned)a->data[24],(unsigned)a->data[25],
     (unsigned)a->data[0x36],(unsigned)a->data[0x81],level,ready);
   if(a->data[0x101]==0x5a){done=1;break;}
 }
 printf("NU019D_SUMMARY mode=%s pad=%d ready=%d done=%d steps=%d transitions=%d "
        "r18=%02x r19=%02x r20=%02x r21=%02x r22=%02x r23=%02x "
        "r24=%02x r25=%02x tifr1=%02x tccr1b=%02x\n",
   names[mode],pad,ready,done,steps,transitions,
   (unsigned)a->data[18],(unsigned)a->data[19],(unsigned)a->data[20],
   (unsigned)a->data[21],(unsigned)a->data[22],(unsigned)a->data[23],
   (unsigned)a->data[24],(unsigned)a->data[25],
   (unsigned)a->data[0x36],(unsigned)a->data[0x81]);
 avr_terminate(a);
 if(!ready||!done)return 6;
 return 0;
}
