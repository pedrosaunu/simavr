/* Independent structural IRQ-entry gate for two-/three-byte AVR PCs.
 * Derived from the Microchip classic-AVR interrupt response specification:
 * 2-byte return PC => four entry clocks; 3-byte => five entry clocks.
 * Injects a synthetic pending vector through simavr's public IRQ API,
 * so it does NOT test real peripheral flag timing or sleep wake-up.
 */
#include "sim_avr.h"
#include "sim_interrupts.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int check(const char *mcu, unsigned expected_pc_bytes, unsigned expected_clocks)
{
    avr_t *a=avr_make_mcu_by_name(mcu);
    if(!a || avr_init(a)) return 2;
    if(a->address_size != expected_pc_bytes) {
        fprintf(stderr,"IRQ_WIDTH_BAD_PC_BYTES mcu=%s actual=%u expected=%u\n",
                mcu,(unsigned)a->address_size,expected_pc_bytes);
        avr_terminate(a); return 1;
    }
    const unsigned native_features=a->cpu_timing_features;
    if (!strcmp(mcu,"atmega328p")) {
        if (!(native_features & AVR_CPU_TIMING_IRQ_ENTRY)) {
            avr_terminate(a); return 1;
        }
    } else {
        if (native_features != 0) {
            fprintf(stderr,"IRQ_WIDTH_BAD_DEFAULT mcu=%s features=%u\n",
                    mcu,native_features);
            avr_terminate(a); return 1;
        }
        /* Exercise shared implementation on a second device WITHOUT
         * changing the production factory configuration for this core.
         */
        a->cpu_timing_features |= AVR_CPU_TIMING_IRQ_ENTRY;
    }
    const unsigned oldsp=(unsigned)a->data[R_SPL] |
                         ((unsigned)a->data[R_SPH] << 8);
    if(oldsp < 16) { avr_terminate(a); return 1; }
    const uint32_t source_pc=0x100;
    const avr_cycle_count_t begin=a->cycle;
    avr_int_vector_t interrupt={
        .vector=61,
        .enable={.reg=0x3e,.bit=0,.mask=1}
    };
    a->pc=source_pc;
    a->sreg[S_I]=1;
    a->data[0x3e] |= 1U;
    avr_register_vector(a, &interrupt);
    if (!avr_raise_interrupt(a,&interrupt)) {
        fprintf(stderr,"IRQ_WIDTH_RAISE_FAILED mcu=%s\n",mcu);
        avr_terminate(a); return 1;
    }
    avr_service_interrupts(a);
    const unsigned newsp=(unsigned)a->data[R_SPL] |
                         ((unsigned)a->data[R_SPH] << 8);
    uint32_t saved_word=0;
    for (unsigned i=0;i<expected_pc_bytes;i++)
        saved_word |= (uint32_t)a->data[oldsp-i] << (8U*i);
    const avr_cycle_count_t observed=a->cycle-begin;
    const int equal=observed==expected_clocks &&
        a->pc==61U * a->vector_size &&
        newsp+expected_pc_bytes==oldsp &&
        saved_word==(source_pc>>1) &&
        a->sreg[S_I]==0;
    printf("IRQ_WIDTH_RESULT mcu=%s pc_bytes=%u cycles=%llu expected=%u "
           "vector_pc=%u sp_before=%u sp_after=%u saved_pc_word=0x%x equal=%d\n",
           mcu,expected_pc_bytes,(unsigned long long)observed,
           expected_clocks,(unsigned)a->pc,oldsp,newsp,
           (unsigned)saved_word,equal);
    avr_terminate(a);
    return equal ? 0 : 1;
}

int main(void)
{
    const int a=check("atmega328p",2,4);
    const int b=check("atmega2560",3,5);
    printf("IRQ_WIDTH_CROSS_DEVICE_RESULT=%s\n",(!a&&!b)?"PASS":"FAIL");
    return a||b;
}
