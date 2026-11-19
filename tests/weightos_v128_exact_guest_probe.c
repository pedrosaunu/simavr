/* Exactly 14 guest instructions before BREAK, same firmware bytes as
 * WeightOS v1.28 neural model. CPU-visible r17/r18 expected independently
 * from Microchip's shared TEMP and OCR direct-read rules.
 */
#include "sim_avr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
int main(int argc,char **argv) {
    if(argc!=3 || (strcmp(argv[2],"--strict") &&
                   strcmp(argv[2],"--negative-r17") &&
                   strcmp(argv[2],"--negative-r18"))) return 2;
    FILE *f=fopen(argv[1],"rb");
    if(!f)return 2;
    if(fseek(f,0,SEEK_END)){fclose(f);return 2;}
    long n=ftell(f);
    if(n<2 || n>128 || n%2 || fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    uint8_t *fw=malloc((size_t)n);if(!fw){fclose(f);return 2;}
    int readok=fread(fw,1,(size_t)n,f)==(size_t)n;
    fclose(f);
    if(!readok){free(fw);return 2;}
    avr_t *a=avr_make_mcu_by_name("atmega328p");
    if(!a || avr_init(a)){free(fw);return 2;}
    a->frequency=16000000;
    avr_loadcode(a,fw,(uint32_t)n,0);
    free(fw);
    unsigned failure=0;
    for(int i=0;i<14;i++) {
        if(avr_run(a)!=cpu_Running){failure++;break;}
    }
    const unsigned observed_r17=a->data[17], observed_r18=a->data[18];
    unsigned expected_r17=0,expected_r18=0x56;
    if(!strcmp(argv[2],"--negative-r17"))expected_r17^=1;
    if(!strcmp(argv[2],"--negative-r18"))expected_r18^=1;
    failure+=(observed_r17!=expected_r17)+(observed_r18!=expected_r18);
    printf("WEIGHTOS_V128_EXACT_GUEST r17=%02x expected=%02x r18=%02x expected=%02x failed=%u cycles=%llu\n",
           observed_r17,expected_r17,observed_r18,expected_r18,
           failure,(unsigned long long)a->cycle);
    avr_terminate(a);
    return failure?1:0;
}
