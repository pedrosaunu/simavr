/* Raw T1 pin discriminator: guest architectural reads after settled levels.
 * No WeightOS code or synchronizer is linked. No silicon phase assertion. */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int retire(avr_t *a) {
    a->run_cycle_limit=1;
    a->run_cycle_count=1;
    return avr_run(a)==cpu_Running;
}
int main(int argc,char **argv) {
    if(argc!=4) return 2;
    unsigned mode=(unsigned)atoi(argv[2]),cs=(unsigned)atoi(argv[3]);
    if(!(mode==1||mode==2||mode==3||mode==8||mode==9||mode==10||mode==11)||(cs!=6&&cs!=7)) return 2;
    unsigned top=mode==1?255:mode==2?511:mode==3?1023:7;
    uint8_t image[1024];
    FILE *f=fopen(argv[1],"rb"); if(!f)return 2;
    size_t n=fread(image,1,sizeof image,f); fclose(f);
    if(!n||n%2||n==sizeof image)return 2;
    avr_t *a=avr_make_mcu_by_name("atmega328p"); if(!a||avr_init(a))return 3;
    a->frequency=16000000; avr_loadcode(a,image,n,0);
    avr_irq_t *pin=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('D'),5);
    if(!pin)return 3;
    avr_raise_irq(pin,0);
    unsigned i;
    for(i=0;i<100 && a->data[25]!=90;i++)if(!retire(a))return 4;
    if(a->data[25]!=90)return 5;
    const unsigned pattern[]={0,1,1,0,0,1,0,1,1,0,1,0,0,1,1,0};
    unsigned old=0,edges=0,matches=0,flag_matches=0;
    for(unsigned k=0;k<64;k++){
        unsigned level=pattern[k%16];
        if(level!=old && ((cs==7&&level)||(cs==6&&!level)))edges++;
        old=level;
        unsigned long long edge_cycle=(unsigned long long)a->cycle;
        avr_raise_irq(pin,level);
        unsigned marker=a->data[24];
        for(i=0;i<32&&a->data[24]==marker;i++)if(!retire(a))return 4;
        if(a->data[24]==marker)return 5;
        unsigned phase=edges%(2*top),expected=phase<=top?phase:2*top-phase;
        unsigned actual=a->data[18]+256u*a->data[19];
        matches+=(actual==expected);
        /* Fixture has OCR1B=2; OCR1A=3 or TOP=7, no W1C/ISR after startup. */
        unsigned expected_flags=(edges>=3?4u:0u) | (edges>=((mode==9||mode==11)?7u:4u)?2u:0u) |
            (((mode==8||mode==10)&&edges>=7)?32u:0u) | ((mode>=8&&edges>=14)?1u:0u);
        flag_matches+=(a->data[20]==expected_flags);
        printf("NU016RZ_SAMPLE mode=%u cs=%u sample=%u level=%u edges=%u edge_cycle=%llu observed_cycle=%llu expected=%u actual=%u flags=%u\n",
            mode,cs,k,level,edges,edge_cycle,(unsigned long long)a->cycle,expected,actual,a->data[20]);
    }
    printf("NU016RZ_REFERENCE mode=%u cs=%u samples=64 edges=%u matches=%u\n",mode,cs,edges,matches);
    printf("NU016SB_REFERENCE mode=%u cs=%u samples=64 counter_matches=%u flag_matches=%u\n",mode,cs,matches,flag_matches);
    avr_terminate(a);return (matches==64 && flag_matches==64)?0:6;
}
