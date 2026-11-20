/* Real firmware through simulator CPU from reset, no vector injection.
 * Output per-instruction PC (word), cycles, r21, SP and SREG.I.
 * This deliberately does not predefine a successful neural comparison.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_STEPS 36

int main(int argc,char**argv) {
    if(argc!=2)return 2;
    FILE*f=fopen(argv[1],"rb");
    if(!f)return 2;
    uint8_t image[32768]={0};
    size_t n=fread(image,1,sizeof(image),f);
    fclose(f);
    if(n<80||n>=sizeof(image)){fprintf(stderr,"invalid image %zu\n",n);return 3;}
    avr_t*a=avr_make_mcu_by_name("atmega328p");
    if(!a||avr_init(a))return 2;
    a->frequency=16000000;
    avr_loadcode(a,image,(uint32_t)n,0);
    unsigned saw_isr=0,saw_reti=0;
    unsigned before_sp=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
    for(unsigned step=1;step<=MAX_STEPS;step++) {
        int rc=avr_run(a);
        if(rc!=cpu_Running&&rc!=cpu_Sleeping) {
            fprintf(stderr,"CPU_STOP step=%u rc=%d pc=%x\n",step,rc,(unsigned)a->pc);
            avr_terminate(a);return 4;
        }
        unsigned sp=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
        unsigned pc_word=(unsigned)a->pc/2U;
        unsigned isr=(pc_word==0x1a);
        if(isr)saw_isr=1;
        if(saw_isr&&sp==before_sp)saw_reti=1;
        printf("SAME_GUEST_IRQ_TRACE step=%u pc_word=%04x cycle=%llu "
               "r21=%02x tifr1=%02x timsk1=%02x I=%u sp_delta=%d\n",
               step,pc_word,(unsigned long long)a->cycle,
               (unsigned)a->data[21],(unsigned)a->data[0x36],
               (unsigned)a->data[0x6f],(unsigned)a->sreg[S_I],
               (int)before_sp-(int)sp);
        if(saw_reti&&a->data[21]==1)break;
    }
    printf("SAME_GUEST_IRQ_SUMMARY isr_entered=%u reti_completed=%u "
           "r21=%u\n",saw_isr,saw_reti,(unsigned)a->data[21]);
    avr_terminate(a);
    return saw_isr&&saw_reti?0:5;
}
