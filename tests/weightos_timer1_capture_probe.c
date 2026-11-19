/* Independent simavr ICP1 guest-visible acceptance diagnostic.
 * No host timer register reads: assertions are only guest AVR GPRs.
 */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { CAP=8192 };
int main(int argc,char **argv){
    if(argc!=3 || (strcmp(argv[2],"--observe")!=0 &&
                   strcmp(argv[2],"--strict")!=0 &&
                   strcmp(argv[2],"--expect-known-red")!=0)) {
        fprintf(stderr,"usage: %s firmware.bin --observe|--strict|--expect-known-red\n",argv[0]);
        return 2;
    }
    uint8_t img[CAP]={0};
    FILE *f=fopen(argv[1],"rb");
    if(!f){perror(argv[1]);return 2;}
    size_t n=fread(img,1,sizeof img,f);
    int err=ferror(f);fclose(f);
    if(err || !n || n>=sizeof img)return 2;
    avr_t *avr=avr_make_mcu_by_name("atmega328p");
    if(!avr || avr_init(avr))return 3;
    avr->frequency=16000000;
    avr_irq_t *pb0=avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('B'),IOPORT_IRQ_PIN0);
    if(!pb0){fprintf(stderr,"Cannot bind PB0 input IRQ\n");avr_terminate(avr);return 3;}
    avr_raise_irq(pb0,0);
    avr_loadcode(avr,img,(uint32_t)n,0);
    unsigned injected=0,finished=0;
    for(unsigned step=0;step<500;step++){
        int status=avr_run(avr);
        if(avr->data[0x101]==0x5a){finished=1;break;}
        if(status!=cpu_Running && status!=cpu_Sleeping && status!=cpu_Done) {
            fprintf(stderr,"MCU stop=%d step=%u pc=0x%x codeend=0x%x r17=%02x r18=%02x r19=%02x\n",status,step,(unsigned)avr->pc,(unsigned)avr->codeend,avr->data[17],avr->data[18],avr->data[19]);
            avr_terminate(avr);return 4;
        }
        if(!injected && avr->data[0x100]==0xa5){
            avr_raise_irq(pb0,1);
            printf("ICP1_EXTERNAL_EDGE cycle=%llu PB0=1\n",
                   (unsigned long long)avr->cycle);
            injected=1;
        }
        if(avr->data[0x101]==0x5a){finished=1;break;}
    }
    if(!finished || !injected){fprintf(stderr,"Firmware edge handshake failed\n");
        avr_terminate(avr);return 5;}
    unsigned low=avr->data[17],high=avr->data[18],flags=avr->data[19];
    unsigned counter_low=avr->data[20],counter_high=avr->data[21];
    int match=(counter_low==0x34 && counter_high==0x12 &&
               low==0x34 && high==0x12 && (flags&0x20)==0x20);
    printf("ICP1_PRE_EDGE_GUEST_TCNT1L=%02x TCNT1H=%02x\n",counter_low,counter_high);
    printf("ICP1_GUEST_READBACK ICR1L=%02x ICR1H=%02x TIFR1=%02x expected_16=1234 match=%d\n",
           low,high,flags,match);
    avr_terminate(avr);
    if(strcmp(argv[2],"--strict")==0)return match?0:1;
    if(strcmp(argv[2],"--expect-known-red")==0) {
        /* Preserves a specific simulator disagreement, not a functional PASS. */
        return !match && low==0 && high==0 &&
               counter_low==0x34 && counter_high==0x12 &&
               (flags&0x20)==0x20 ? 0 : 9;
    }
    return 0;
}
