/* Fail-closed two-firmware shared /8 prescaler TSM/PSRSYNC differential.
 * No direct simavr timer register backing reads: only guest SRAM snapshots
 * produced by AVR IN/LDS instructions in the firmware are inspected.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned held0[2], held1[2], gtccr, after[2], later[2], cycles;
} capture_t;

static int observe(const char *path, capture_t *out) {
    FILE *f=fopen(path,"rb");
    if(!f) { perror(path); return 2; }
    uint8_t image[32768]={0};
    size_t count=fread(image,1,sizeof(image),f);
    fclose(f);
    if(!count || count==sizeof(image)) return 2;
    avr_t *a=avr_make_mcu_by_name("atmega328p");
    if(!a || avr_init(a)) return 3;
    a->frequency=16000000;
    avr_loadcode(a,image,(uint32_t)count,0);
    int done=0;
    for(unsigned i=0;i<2000;i++) {
        int state=avr_run(a);
        if(a->data[0x100]==0xa5) { done=1; break; }
        if(state!=cpu_Running) break;
    }
    if(done) {
        out->held0[0]=a->data[0x110];out->held0[1]=a->data[0x111];
        out->held1[0]=a->data[0x112];out->held1[1]=a->data[0x113];
        out->gtccr=a->data[0x114];
        out->after[0]=a->data[0x115];out->after[1]=a->data[0x116];
        out->later[0]=a->data[0x117];out->later[1]=a->data[0x118];
        out->cycles=(unsigned)a->cycle;
    }
    avr_terminate(a);
    return done?0:4;
}

static void report(const char *label,const capture_t *x) {
    printf("GTCCR_GUEST %s held0=%u,%u held1=%u,%u gtccr=%02x after=%u,%u later=%u,%u cycles=%u\n",
        label,x->held0[0],x->held0[1],x->held1[0],x->held1[1],
        x->gtccr,x->after[0],x->after[1],x->later[0],x->later[1],x->cycles);
}

int main(int argc,char **argv) {
    if(argc!=4) {
        fprintf(stderr,"usage: probe nop3.bin nop11.bin --strict|--observe\n");
        return 2;
    }
    capture_t a={0},b={0};
    int x=observe(argv[1],&a),y=observe(argv[2],&b);
    if(x||y) {
        fprintf(stderr,"GTCCR_GUEST_ERROR first=%d second=%d\n",x,y);
        return 3;
    }
    report("nop3",&a);report("nop11",&b);
    unsigned hold_ok=(a.held0[0]==0&&a.held0[1]==0&&
        a.held1[0]==0&&a.held1[1]==0&&a.gtccr==0x81&&
        b.held0[0]==0&&b.held0[1]==0&&b.held1[0]==0&&
        b.held1[1]==0&&b.gtccr==0x81);
    unsigned delta_ok=1,shared_ok=1;
    for(int i=0;i<2;i++) {
        if(b.after[i]!=a.after[i]+1 || b.later[i]!=a.later[i]+1) delta_ok=0;
        if(a.later[i]<a.after[i]||b.later[i]<b.after[i]) delta_ok=0;
    }
    if(abs((int)a.after[0]-(int)a.after[1])>1 ||
       abs((int)a.later[0]-(int)a.later[1])>1 ||
       abs((int)b.after[0]-(int)b.after[1])>1 ||
       abs((int)b.later[0]-(int)b.later[1])>1) shared_ok=0;
    printf("GTCCR_STRICT_REFERENCE_SUMMARY hold=%u eight_cpu_cycle_delta=%u simultaneous_timer0_timer1=%u\n",
        hold_ok,delta_ok,shared_ok);
    if(strcmp(argv[3],"--observe")==0) return 0;
    return hold_ok&&delta_ok&&shared_ok?0:1;
}
