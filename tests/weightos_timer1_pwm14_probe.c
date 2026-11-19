/* Independent simulator output-pin diagnostic, no WeightOS dependency.
 * Observe public IO-port state, never internal timer state.
 */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { CAP=8192, SAMPLES=32 };
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr,"usage: %s firmware.bin\n",argv[0]); return 2; }
    uint8_t image[CAP]={0};
    FILE *f=fopen(argv[1],"rb");
    if (!f) { perror(argv[1]); return 2; }
    size_t n=fread(image,1,sizeof image,f);
    int err=ferror(f); fclose(f);
    if (err || !n || n>=sizeof image) return 2;
    avr_t *avr=avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr)) return 3;
    avr->frequency=16000000;
    avr_loadcode(avr,image,(uint32_t)n,0);
    unsigned got=0;
    unsigned long long prev=0;
    for(unsigned step=0; step<500 && got<SAMPLES; step++){
        int status=avr_run(avr);
        if(status!=cpu_Running && status!=cpu_Sleeping){
            fprintf(stderr,"MCU status=%d step=%u\n",status,step);
            avr_terminate(avr); return 4;
        }
        if(avr->data[0x100]!=0xa5) continue; /* marker only, not IO oracle */
        avr_ioport_state_t port={0};
        if(avr_ioctl(avr,AVR_IOCTL_IOPORT_GETSTATE('B'),&port)!=0){
            fprintf(stderr,"IOPORT_GETSTATE failed\n");
            avr_terminate(avr); return 5;
        }
        const unsigned long long cycle=(unsigned long long)avr->cycle;
        if(got && cycle!=prev+1){
            fprintf(stderr,"Non-single-cycle sampling gap %llu to %llu\n",prev,cycle);
            avr_terminate(avr); return 6;
        }
        printf("PWM14_PIN_SAMPLE i=%u cycle=%llu port=%02lx ddr=%02lx pin=%02lx\n",
               got,cycle,(unsigned long)port.port,(unsigned long)port.ddr,
               (unsigned long)port.pin);
        prev=cycle;
        got++;
    }
    printf("PWM14_PIN_SUMMARY samples=%u\n",got);
    avr_terminate(avr);
    return got==SAMPLES?0:7;
}
