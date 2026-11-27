/* Same guest ROM and settled stimulus as NU-016SD neural positive run.
 * No physical synchronizer or unsafe-switch policy claim. */
#include "sim_avr.h"
#include "avr_ioport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int retire(avr_t *a) {
    a->run_cycle_limit=1; a->run_cycle_count=1;
    return avr_run(a)==cpu_Running;
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    uint8_t image[1024];
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(image,1,sizeof image,f);fclose(f);
    if(!n||n%2||n==sizeof image)return 2;
    avr_t *a=avr_make_mcu_by_name("atmega328p");if(!a||avr_init(a))return 3;
    a->frequency=16000000;avr_loadcode(a,image,n,0);
    avr_irq_t *pin=avr_io_getirq(a,AVR_IOCTL_IOPORT_GETIRQ('D'),5);
    if(!pin)return 3;
    avr_raise_irq(pin,0);
    unsigned i;
    for(i=0;i<100&&a->data[25]!=90;i++)if(!retire(a))return 4;
    if(a->data[25]!=90)return 5;
    const unsigned pattern[]={0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,0};
    const unsigned select[]={7,6,0,7};
    unsigned old=0,edges=0,matches=0;
    for(unsigned k=0;k<64;k++) {
        unsigned stage=k/16,cs=select[stage],level=pattern[k%16];
        if(level!=old&&((cs==7&&level)||(cs==6&&!level)))edges++;
        old=level;avr_raise_irq(pin,level);
        unsigned marker=a->data[24];
        for(i=0;i<48&&a->data[24]==marker;i++)if(!retire(a))return 4;
        if(a->data[24]==marker)return 5;
        unsigned phase=edges%14,expected=phase<=7?phase:14-phase;
        unsigned flags=(edges>=3?4u:0u)|(edges>=4?2u:0u)|(edges>=7?32u:0u)|(edges>=14?1u:0u);
        unsigned actual=a->data[18]+256u*a->data[19];
        matches+=(actual==expected&&a->data[20]==flags&&a->data[23]==stage&&a->data[24]==k+1);
        printf("NU016SD_REF_SAMPLE sample=%u stage=%u guest_stage=%u marker=%u cs=%u level=%u edges=%u expected=%u actual=%u expected_flags=%u flags=%u cycle=%llu\n",
            k,stage,a->data[23],a->data[24],cs,level,edges,expected,actual,flags,a->data[20],(unsigned long long)a->cycle);
    }
    printf("NU016SD_REFERENCE samples=64 matches=%u\n",matches);
    avr_terminate(a);return matches==64?0:6;
}
