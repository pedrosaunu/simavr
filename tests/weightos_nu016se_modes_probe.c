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
    if(!(mode==0||mode==4||mode==5||mode==6||mode==7||mode==12||mode==14||mode==15)||(cs!=6&&cs!=7)) return 2;
    unsigned top=mode==0?65535:mode==5?255:mode==6?511:mode==7?1023:7;
    unsigned seed=(mode==0||mode==5||mode==6||mode==7)?top-3:0;
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
    unsigned old=0,edges=0,matches=0;
    for(unsigned k=0;k<64;k++){
        unsigned level=pattern[k%16];
        if(level!=old && ((cs==7&&level)||(cs==6&&!level)))edges++;
        old=level;
        unsigned long long edge_cycle=(unsigned long long)a->cycle;
        avr_raise_irq(pin,level);
        unsigned marker=a->data[24];
        for(i=0;i<32&&a->data[24]==marker;i++)if(!retire(a))return 4;
        if(a->data[24]==marker)return 5;
        unsigned expected=(seed+edges)%(top+1);
        unsigned actual=a->data[18]+256u*a->data[19];
        matches+=(actual==expected);
        printf("NU016SE_SAMPLE mode=%u cs=%u sample=%u level=%u edges=%u edge_cycle=%llu observed_cycle=%llu expected=%u actual=%u flags=%u\n",
            mode,cs,k,level,edges,edge_cycle,(unsigned long long)a->cycle,expected,actual,a->data[20]);
    }
    printf("NU016SE_REFERENCE mode=%u cs=%u samples=64 edges=%u matches=%u\n",mode,cs,edges,matches);
    avr_terminate(a);return 0;
}
