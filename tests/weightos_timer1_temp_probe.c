/* Independently derived Timer1 TEMP transaction order, from ATmega328/P
 * 16-bit I/O access specification. This fixture tests stopped Timer1
 * only; it does not claim correctness under active prescaling or PWM.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv) {
    if(argc!=3 || (strcmp(argv[2],"--strict") &&
                     strcmp(argv[2],"--observe"))) {
        fprintf(stderr,"usage: %s firmware.bin --strict|--observe\n",argv[0]);
        return 2;
    }
    FILE *f=fopen(argv[1],"rb");
    if(!f) return 2;
    if(fseek(f,0,SEEK_END)){fclose(f);return 2;}
    const long n=ftell(f);
    if(n<6||n>512||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    uint8_t *fw=malloc((size_t)n);
    if(!fw){fclose(f);return 2;}
    const size_t used=fread(fw,1,(size_t)n,f);
    fclose(f);
    if(used!=(size_t)n){free(fw);return 2;}
    avr_t *a=avr_make_mcu_by_name("atmega328p");
    if(!a||avr_init(a)){free(fw);return 2;}
    a->frequency=16000000;
    avr_loadcode(a,fw,(uint32_t)n,0);
    unsigned steps=0;
    while(steps++<200 && a->data[0x3e]!=0xa5) {
        const int status=avr_run(a);
        if(status!=cpu_Running) break;
    }
    const uint8_t expected[6]={0x34,0x12,0x78,0x12,0xcd,0xab};
    unsigned mismatches=0;
    for(unsigned i=0;i<6;i++) {
        const unsigned actual=a->data[17+i];
        const int ok=actual==expected[i];
        mismatches+=!ok;
        printf("TIMER1_TEMP reg=r%u actual=0x%02x expected=0x%02x equal=%d\n",
               17+i,actual,(unsigned)expected[i],ok);
    }
    const int finished=a->data[0x3e]==0xa5;
    if(!finished) mismatches++;
    printf("TIMER1_TEMP_SUMMARY finished=%d cycles=%llu mismatches=%u "
           "timsk1=0x%02x tccr1b=0x%02x\n",
           finished,(unsigned long long)a->cycle,mismatches,
           (unsigned)a->data[0x6f],(unsigned)a->data[0x81]);
    avr_terminate(a);
    free(fw);
    return strcmp(argv[2],"--observe") ? (mismatches!=0) : !finished;
}
