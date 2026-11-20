/* Actual AVR firmware ISR delivery with 3 compiled TIMSK1 masks.
 * The test does not change interrupt vectors/flags through host APIs.
 * Capture boundary-visible PC word/cycles/register values for later
 * independent comparison with weight-native execution.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STEPS 40
static int run(const char*path,unsigned mask) {
    uint8_t image[32768]={0};
    FILE*f=fopen(path,"rb");
    if(!f){perror(path);return 2;}
    size_t n=fread(image,1,sizeof(image),f);fclose(f);
    if(n<70||n>=sizeof(image))return 2;
    avr_t*a=avr_make_mcu_by_name("atmega328p");
    if(!a||avr_init(a))return 2;
    a->frequency=16000000;
    avr_loadcode(a,image,(uint32_t)n,0);
    unsigned old_sp=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
    unsigned seen_a=0,seen_b=0,returned_a=0,returned_b=0,first_vector=0;
    unsigned finish=0,steps=0;
    for(unsigned i=0;i<MAX_STEPS;i++){
        int rc=avr_run(a);
        if(rc!=cpu_Running&&rc!=cpu_Sleeping){
            fprintf(stderr,"COMPARE_GUEST_HALTED mask=%u rc=%d\n",mask,rc);
            avr_terminate(a);return 3;
        }
        unsigned pc=(unsigned)a->pc/2U;
        unsigned sp=(unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
        if(pc==0x16){seen_a=1;if(!first_vector)first_vector=0x16;}
        if(pc==0x18){seen_b=1;if(!first_vector)first_vector=0x18;}
        if(seen_a&&sp==old_sp&&a->data[21]==1)returned_a=1;
        if(seen_b&&sp==old_sp&&a->data[22]==1)returned_b=1;
        steps=i+1;
        printf("TIMER1_COMPARE_GUEST_TRACE mask=%02x step=%u pc_word=%04x "
               "cycle=%llu r21=%02x r22=%02x r23=%02x "
               "tifr1=%02x timsk1=%02x I=%u sp_delta=%d\n",
               mask,steps,pc,(unsigned long long)a->cycle,
               (unsigned)a->data[21],(unsigned)a->data[22],
               (unsigned)a->data[23],(unsigned)a->data[0x36],
               (unsigned)a->data[0x6f],(unsigned)a->sreg[S_I],
               (int)old_sp-(int)sp);
        if((!(mask&2)||returned_a)&&(!(mask&4)||returned_b)){
            finish=1;break;
        }
    }
    unsigned expected_first=(mask&2)?0x16:0x18;
    int ok=finish&&first_vector==expected_first &&
        a->data[21]==((mask&2)?1:0)&&
        a->data[22]==((mask&4)?1:0)&&
        a->data[23]==0&&a->data[0x6f]==mask&&
        a->data[0x36]==(6U&~mask);
    printf("TIMER1_COMPARE_GUEST_SUMMARY mask=%02x steps=%u first_vector=%04x "
           "compa_isr=%u compb_isr=%u reti_a=%u reti_b=%u "
           "r21=%u r22=%u tifr1=%02x pass=%d\n",
           mask,steps,first_vector,seen_a,seen_b,returned_a,returned_b,
           (unsigned)a->data[21],(unsigned)a->data[22],
           (unsigned)a->data[0x36],ok);
    avr_terminate(a);
    return ok?0:1;
}
int main(int argc,char**argv){
    if(argc!=3)return 2;
    unsigned mask=(unsigned)strtoul(argv[2],NULL,0);
    if(mask!=2&&mask!=4&&mask!=6)return 2;
    return run(argv[1],mask);
}
