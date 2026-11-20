/* Observe W1C exclusively through real guest AVR instructions.
 * Use register writes and guest IN snapshots in r20..r23,
 * with simulator PC/cycles/TCNT1/TIFR1 as differential evidence.
 */
#include "sim_avr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MAX_STEPS 42

int main(int argc,char**argv){
 if(argc!=3)return 2;
 unsigned style=(unsigned)strtoul(argv[2],NULL,0);
 if(style>2)return 2;
 uint8_t image[32768]={0};
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(image,1,sizeof(image),f);fclose(f);
 if(n<70||n>=sizeof(image))return 2;
 avr_t*a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 2;
 a->frequency=16000000;avr_loadcode(a,image,(uint32_t)n,0);
 int seen_first=0,seen_stop=0,matched=0;
 unsigned now=0;
 for(unsigned step=1;step<=MAX_STEPS;step++){
    int rc=avr_run(a);
    if(rc!=cpu_Running&&rc!=cpu_Sleeping)break;
    unsigned pc=(unsigned)a->pc/2U;
    if(pc>=0x29){seen_first=1;}
    if(!a->data[0x81]&&seen_first)seen_stop=1;
    now=(unsigned)a->data[0x84]+((unsigned)a->data[0x85]<<8);
    printf("TIMER1_W1C_GUEST_TRACE style=%u step=%u pc_word=%04x cycle=%llu "
        "r20=%02x r21=%02x r22=%02x r23=%02x tifr1=%02x "
        "tccr1b=%02x tcnt1=%04x\n",
        style,step,pc,(unsigned long long)a->cycle,
        (unsigned)a->data[20],(unsigned)a->data[21],
        (unsigned)a->data[22],(unsigned)a->data[23],
        (unsigned)a->data[0x36],(unsigned)a->data[0x81],now);
    if(pc==(style==0?0x32:0x31)){
        const unsigned after=(unsigned)a->data[21];
        const unsigned final=(unsigned)a->data[23];
        matched=(a->data[20]==6 && a->data[22]==after && seen_stop &&
                 ((style==0 && after==4 && final==0) ||
                  (style==1 && after==0 && final==0) ||
                  (style==2 && after==2 && final==2)));
        break;
    }
 }
 printf("TIMER1_W1C_GUEST_SUMMARY style=%u before=%02x after1=%02x after2=%02x "
        "after_zero=%02x timer_stopped=%u pass=%d\n",
       style,(unsigned)a->data[20],(unsigned)a->data[21],
       (unsigned)a->data[22],(unsigned)a->data[23],seen_stop,matched);
 avr_terminate(a);return matched?0:5;
}
