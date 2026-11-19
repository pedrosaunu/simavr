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

typedef struct {
    unsigned bit, level, updates;
    avr_t *avr;
} pwm_observer_t;
static void pwm_pin_observe(struct avr_irq_t *irq, uint32_t value, void *param){
    (void)irq;
    pwm_observer_t *p=(pwm_observer_t*)param;
    p->level=(value & 1U);
    p->updates++;
    if(p->updates<20)
        printf("PWM14_IRQ_EVENT pin=%u cycle=%llu value=%u\n",p->bit,
               (unsigned long long)p->avr->cycle,p->level);
}

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
    pwm_observer_t a={.bit=1,.avr=avr}, b={.bit=2,.avr=avr};
    avr_irq_t *pa=avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN1);
    avr_irq_t *pb=avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN2);
    if(!pa || !pb){
        fprintf(stderr,"Failed to bind PB1/PB2 external output IRQ\n");
        avr_terminate(avr); return 8;
    }
    avr_irq_register_notify(pa,pwm_pin_observe,&a);
    avr_irq_register_notify(pb,pwm_pin_observe,&b);
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
        printf("PWM14_PIN_SAMPLE i=%u cycle=%llu port=%02lx ddr=%02lx pin=%02lx irq=%02x\n",
               got,cycle,(unsigned long)port.port,(unsigned long)port.ddr,
               (unsigned long)port.pin,(a.level?2U:0U)|(b.level?4U:0U));
        prev=cycle;
        got++;
    }
    printf("PWM14_PIN_SUMMARY samples=%u\n",got);
    avr_terminate(avr);
    return got==SAMPLES?0:7;
}
