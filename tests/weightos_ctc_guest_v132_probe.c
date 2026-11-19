/* Live Timer1 guest-observable CTC diagnostic.
 * Reports GPRs only, never avr->data[TCNT1] as a timer oracle.
 * Does not assume the expected counter sequence; timing is to be reconciled.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static int load(avr_t *a,const char *path) {
    FILE *f=fopen(path,"rb"); if(!f)return 0;
    if(fseek(f,0,SEEK_END))return fclose(f),0;
    long n=ftell(f); if(n<2||n>512||n%2||fseek(f,0,SEEK_SET))return fclose(f),0;
    uint8_t *buf=malloc((size_t)n);if(!buf)return fclose(f),0;
    int ok=fread(buf,1,(size_t)n,f)==(size_t)n;fclose(f);
    if(ok)avr_loadcode(a,buf,(uint32_t)n,0);
    free(buf);return ok;
}
int main(int argc,char **argv) {
    if(argc!=3)return fprintf(stderr,"usage: %s firmware.bin wgm4|wgm12\n",argv[0]),2;
    avr_t *a=avr_make_mcu_by_name("atmega328p");
    if(!a||avr_init(a)||!load(a,argv[1]))return 2;
    a->frequency=16000000;
    unsigned steps=0;
    while(steps++<120 && a->data[0x3e]!=0xa5) {
        if(avr_run(a)!=cpu_Running)break;
    }
    int done=a->data[0x3e]==0xa5;
    printf("CTC_GUEST mode=%s done=%d steps=%u cycle=%llu r17=%u r18=%u r19=%u r20=%u r21=%u r22=%u\n",
           argv[2],done,steps,(unsigned long long)a->cycle,
           a->data[17],a->data[18],a->data[19],a->data[20],a->data[21],a->data[22]);
    avr_terminate(a);
    return done?0:1;
}
