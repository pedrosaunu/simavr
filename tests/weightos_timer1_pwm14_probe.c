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
static const unsigned expected_pwm[4]={0x06,0x04,0x00,0x00};
static const unsigned known_simavr[4]={0x06,0x04,0x00,0x06};

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
    if(argc != 3 ||
       (strcmp(argv[2],"--observe")!=0 &&
        strcmp(argv[2],"--strict")!=0 &&
        strcmp(argv[2],"--expect-known-red")!=0)) {
        fprintf(stderr,"usage: %s firmware.bin --observe|--strict|--expect-known-red\n",argv[0]);
        return 2;
    }
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
    unsigned measured[SAMPLES]={0};
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
        measured[got]=(a.level?2U:0U)|(b.level?4U:0U);
        printf("PWM14_PIN_SAMPLE i=%u cycle=%llu port=%02lx ddr=%02lx pin=%02lx irq=%02x\n",
               got,cycle,(unsigned long)port.port,(unsigned long)port.ddr,
               (unsigned long)port.pin,(a.level?2U:0U)|(b.level?4U:0U));
        prev=cycle;
        got++;
    }
    if(got!=SAMPLES){ avr_terminate(avr); return 7; }
    unsigned best=0,best_shift=0,knownred=1;
    for(unsigned shift=0;shift<4;shift++){
        unsigned match=0;
        for(unsigned i=0;i<SAMPLES;i++)
            match += measured[i]==expected_pwm[(i+shift)%4];
        if(match>best){best=match;best_shift=shift;}
    }
    for(unsigned i=0;i<SAMPLES;i++)
        if(measured[i]!=known_simavr[i%4]) knownred=0;
    int full=best==SAMPLES;
    /*
     * The startup interval begins during the first asynchronously armed
     * PWM cycle. Demand exact steady-state equality after one 4-cycle
     * period, separately recording whether startup also matches.
     */
    unsigned steady_matches=0;
    for(unsigned i=4;i<SAMPLES;i++)
        steady_matches += measured[i]==expected_pwm[(i+best_shift)%4];
    const int stable=(steady_matches==SAMPLES-4);
    printf("PWM14_REFERENCE_SUMMARY samples=%u expected_matches=%u best_phase=%u exact=%d stable_matches=%u stable_exact=%d startup_exact=%d known_simavr=%d\n",
           got,best,best_shift,full,steady_matches,stable,full,knownred);
    avr_terminate(avr);
    if(strcmp(argv[2],"--observe")==0)return 0;
    if(strcmp(argv[2],"--strict")==0)return stable?0:1;
    /* Diagnostic inverse gate: never confuse this known RED with a PASS. */
    return !full && knownred?0:9;
}
