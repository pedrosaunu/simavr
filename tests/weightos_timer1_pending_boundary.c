/* Timer1 pending interrupt arbitration at a controlled instruction boundary.
 * The simulator's *registered* peripheral vectors are raised through
 * avr_raise_interrupt: never poke the interrupt FIFO or manually select a PC.
 * This is not a guest-firmware-origin event, nor an end-to-end timing proof.
 */
#include "sim_avr.h"
#include "sim_interrupts.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

enum { TIFR1_ADDR=0x36, TIMSK1_ADDR=0x6f, MAX_STEPS=25 };
typedef struct {
    const char *label;
    uint8_t pending, mask, expected_serviced;
    uint8_t with_timer0;
} case_t;
static const case_t cases[]={
    {"capture-first",0x27,0x27,5,0},
    {"compa-first",0x07,0x07,1,0},
    {"compb-first",0x05,0x05,2,0},
    {"overflow-only",0x01,0x01,0,0},
    {"masked-capture",0x27,0x06,1,0},
    {"masked-all-but-b",0x27,0x04,2,0},
    {"timer1-before-timer0",0x02,0x02,1,1},
};
static avr_int_vector_t *find_vector(avr_t *avr,unsigned reg,unsigned bit) {
    for (unsigned i=0;i<(unsigned)avr->interrupts.vector_count;i++) {
        avr_int_vector_t *v=avr->interrupts.vector[i];
        if (v->raised.reg==reg && v->raised.bit==bit)
            return v;
    }
    return NULL;
}
static unsigned sp_value(avr_t *a) {
    return (unsigned)a->data[0x5d]+256U*(unsigned)a->data[0x5e];
}
static int run_case(const case_t *t) {
    uint8_t image[1024]={0}; /* 512 guest AVR NOP instructions */
    avr_t *a=avr_make_mcu_by_name("atmega328p");
    if (!a || avr_init(a)) return 2;
    a->frequency=16000000;
    avr_loadcode(a,image,(uint32_t)sizeof image,0);
    a->data[TIMSK1_ADDR]=t->mask;
    if(t->with_timer0)a->data[0x6e]=1;
    a->sreg[S_I]=1;
    const unsigned before_sp=sp_value(a);
    if (before_sp<2) {avr_terminate(a);return 3;}
    /* Raise in deliberately *reverse* hardware priority order, to test
       arbitration rather than FIFO order. Include masked flags as inputs. */
    for (unsigned bit=0;bit<8;bit++) {
        if(!(t->pending&(1U<<bit)))continue;
        avr_int_vector_t *v=find_vector(a,TIFR1_ADDR,bit);
        if(!v) {
            fprintf(stderr,"missing Timer1 vector bit %u\n",bit);
            avr_terminate(a);
            return 3;
        }
        avr_raise_interrupt(a,v);
    }
    if(t->with_timer0) {
        avr_int_vector_t *v=find_vector(a,0x35,0);
        if (!v) {avr_terminate(a);return 3;}
        avr_raise_interrupt(a,v);
    }
    const unsigned wanted_vector_word=
        t->expected_serviced==5?0x14:
        t->expected_serviced==1?0x16:
        t->expected_serviced==2?0x18:0x1a;
    const unsigned expected_pc_bytes=wanted_vector_word*2;
    const uint8_t expected_flags=t->pending & (uint8_t)~(1U<<t->expected_serviced);
    int reached=0; unsigned selected_pc=0,after_sp=0,flags=0,I=0;
    unsigned cycles=0, saved_pc=0;
    for(unsigned step=0;step<MAX_STEPS;step++) {
        int rc=avr_run(a);
        if (rc!=cpu_Running && rc!=cpu_Sleeping)break;
        const unsigned pc=(unsigned)a->pc;
        if(pc==expected_pc_bytes) {
            reached=1;selected_pc=pc;
            after_sp=sp_value(a);
            flags=a->data[TIFR1_ADDR];
            I=(unsigned)a->sreg[S_I];
            cycles=(unsigned)a->cycle;
            saved_pc=(unsigned)a->data[before_sp] |
                ((unsigned)a->data[before_sp-1]<<8);
            break;
        }
    }
    const int pass=reached &&
        selected_pc==expected_pc_bytes &&
        after_sp==before_sp-2 &&
        flags==expected_flags && I==0 &&
        (!t->with_timer0 || (a->data[0x35]&1));
    printf("TIMER1_PRIORITY_BOUNDARY case=%s pending=%02x mask=%02x "
           "expected_byte_pc=%04x observed_byte_pc=%04x "
           "expected_flags=%02x observed_flags=%02x "
           "sp_delta=%d I=%u saved_pc=%04x cycle=%u pass=%d\n",
           t->label,t->pending,t->mask,expected_pc_bytes,selected_pc,
           expected_flags,flags,(int)before_sp-(int)after_sp,I,saved_pc,cycles,pass);
    avr_terminate(a);
    return pass?0:1;
}
int main(void) {
    unsigned passed=0;
    const unsigned n=(unsigned)(sizeof(cases)/sizeof(cases[0]));
    for(unsigned i=0;i<n;i++)
        if(!run_case(&cases[i]))passed++;
    printf("TIMER1_PRIORITY_BOUNDARY_SUMMARY cases=%u passed=%u failed=%u\n",
        n,passed,n-passed);
    return passed==n?0:1;
}
