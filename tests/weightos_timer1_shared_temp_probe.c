/* Compare only guest-visible GPRs after real AVR instructions.
 * No reading av->data[OCR/TCNT/ICR] or simulator timer internals as oracle.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int load(avr_t *a, const char *path) {
    FILE *f=fopen(path,"rb"); if(!f) return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<6 || n>512 || n%2 || fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    uint8_t *buf=malloc((size_t)n);if(!buf){fclose(f);return 0;}
    int ok=fread(buf,1,(size_t)n,f)==(size_t)n; fclose(f);
    if(ok)avr_loadcode(a,buf,(uint32_t)n,0);
    free(buf);return ok;
}
int main(int argc, char **argv) {
    if(argc!=3 || (strcmp(argv[2],"--strict") &&
                   strcmp(argv[2],"--negative-ocr") &&
                   strcmp(argv[2],"--negative-icr") &&
                   strcmp(argv[2],"--negative-temp"))) {
        fprintf(stderr,"usage: %s guest.bin --strict|--negative-ocr|--negative-icr|--negative-temp\n",argv[0]);
        return 2;
    }
    avr_t *a=avr_make_mcu_by_name("atmega328p");
    if(!a || avr_init(a) || !load(a,argv[1]))return 2;
    a->frequency=16000000;
    unsigned steps=0;
    while(steps++<160 && a->data[0x3e]!=0xa5) {
        int state=avr_run(a);
        if(state!=cpu_Running) break;
    }
    const int finished=a->data[0x3e]==0xa5;
    /* r17..r25: OCR direct/low/high, ICR low/high, OCR direct,
     * TEMP after OCR read, ICR read-latch, shared TCNT high read.
     */
    uint8_t expected[]={0x00,0x34,0x12,0x78,0x56,0xab,0xee,0x78,0x56};
    if(!strcmp(argv[2],"--negative-ocr"))expected[2]^=1;
    if(!strcmp(argv[2],"--negative-icr"))expected[4]^=1;
    if(!strcmp(argv[2],"--negative-temp"))expected[6]^=1;
    unsigned failures=!finished;
    for(unsigned i=0;i<sizeof(expected);i++) {
        unsigned actual=a->data[17+i];
        int pass=actual==expected[i];
        failures+=(unsigned)!pass;
        printf("TIMER1_SHARED_TEMP r%u actual=0x%02x expected=0x%02x equal=%d\n",
               17+i,actual,(unsigned)expected[i],pass);
    }
    printf("TIMER1_SHARED_TEMP_SUMMARY finished=%d cycles=%llu failures=%u steps=%u\n",
           finished,(unsigned long long)a->cycle,failures,steps);
    avr_terminate(a);
    return failures?1:0;
}
