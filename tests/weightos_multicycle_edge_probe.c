/* Host-clock ordering experiment: pending events inside a multi-cycle
 * store must execute before its visible write, despite the instruction
 * completing at a later clock. This is a CPU/peripheral causality test,
 * NOT proof of timer silicon fidelity.
 */
#include "sim_avr.h"
#include "sim_io.h"
#include "sim_cycle_timers.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ADDR = 0x3e };
typedef struct {
    unsigned ticks;
    unsigned writes;
    uint64_t tick_when[2];
    uint64_t tick_cycle[2];
    uint8_t tick_value[2];
    uint64_t write_cycle;
    uint8_t write_value;
} observer_t;

static avr_cycle_count_t event_common(avr_t *a, avr_cycle_count_t when, void *arg) {
    observer_t *o = (observer_t *)arg;
    const unsigned i=o->ticks++;
    if (i < 2) {
        o->tick_when[i] = when;
        o->tick_cycle[i] = a->cycle;
        o->tick_value[i] = a->data[ADDR];
    }
    printf("EDGE_EVENT index=%u when=%llu clock=%llu value=0x%02x\n",
           i + 1, (unsigned long long)when,
           (unsigned long long)a->cycle, (unsigned)a->data[ADDR]);
    return 0;
}

static avr_cycle_count_t event_before(avr_t *a, avr_cycle_count_t when, void *arg) {
    return event_common(a,when,arg);
}
static avr_cycle_count_t event_after(avr_t *a, avr_cycle_count_t when, void *arg) {
    return event_common(a,when,arg);
}

static void write_io(avr_t *a, avr_io_addr_t addr, uint8_t v, void *arg) {
    observer_t *o=(observer_t *)arg;
    o->writes++;
    o->write_cycle=a->cycle;
    o->write_value=v;
    avr_core_watch_write(a, addr, v);
    printf("EDGE_WRITE clock=%llu value=0x%02x\n",
           (unsigned long long)a->cycle, (unsigned)v);
}

int main(int argc,char **argv) {
    if(argc!=3 || (strcmp(argv[2],"--strict") &&
                    strcmp(argv[2],"--expect-known-red"))) {
        fprintf(stderr,"usage: %s file.bin --strict|--expect-known-red\n",argv[0]);
        return 2;
    }
    FILE *f=fopen(argv[1],"rb");
    if(!f) return 2;
    if(fseek(f,0,SEEK_END)) {fclose(f);return 2;}
    const long n=ftell(f);
    if(n<6||n>512||fseek(f,0,SEEK_SET)) {fclose(f);return 2;}
    uint8_t *fw=malloc((size_t)n);
    if(!fw) {fclose(f);return 2;}
    const size_t count=fread(fw,1,(size_t)n,f);
    fclose(f);
    if(count!=(size_t)n){free(fw);return 2;}
    avr_t *a=avr_make_mcu_by_name("atmega328p");
    if(!a||avr_init(a)){free(fw);return 2;}
    a->frequency=16000000;
    avr_loadcode(a,fw,(uint32_t)n,0);
    observer_t o={0};
    avr_register_io_write(a, ADDR,write_io,&o);
    avr_cycle_timer_register(a,2,event_before,&o);
    avr_cycle_timer_register(a,4,event_after,&o);
    for(unsigned step=0;step<20 && (o.ticks<2||o.writes<1);step++) {
        if(avr_run(a)!=cpu_Running) break;
    }
    const int strict=o.ticks==2&&o.writes==1 &&
      o.write_cycle==3&&o.write_value==0xa5 &&
      o.tick_when[0]==2 && o.tick_when[1]==4 &&
      o.tick_cycle[0]==2 && o.tick_cycle[1]==4 &&
      o.tick_value[0]==0 && o.tick_value[1]==0xa5;
    const int known_red=o.ticks==2&&o.writes==1 &&
      o.write_cycle==3 && o.tick_when[0]==2 &&
      o.tick_cycle[0]==3 && o.tick_value[0]==0xa5 &&
      o.tick_when[1]==4 && o.tick_cycle[1]==4 &&
      o.tick_value[1]==0xa5;
    printf("EDGE_ORDER_SUMMARY strict=%d known_red=%d ticks=%u writes=%u\n",
           strict,known_red,o.ticks,o.writes);
    avr_terminate(a);
    free(fw);
    return !strcmp(argv[2],"--strict") ? !strict : !known_red;
}
