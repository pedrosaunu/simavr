/* WeightOS v1.26: independently execute the exact 320-byte dual-timer
 * firmware against the currently linked libsimavr. Read architectural
 * state after each native avr_run(); do not infer timer states from C host code.
 * The designated byte initializers reproduce the archived multi_irq.bin.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>

static const uint8_t firmware[320] = {
  [0]=0x7f, [1]=0xc0, [52]=0x79, [53]=0xc0, [64]=0x7c, [65]=0xc0,
  [256]=0x0f, [257]=0xef, [258]=0x04, [259]=0xb9, [260]=0x01, [261]=0xe0,
  [263]=0x93, [264]=0x6f, [267]=0x93, [268]=0x6e, [270]=0x05, [271]=0xbd,
  [273]=0x93, [274]=0x81, [276]=0x0f, [277]=0xef, [279]=0x93, [280]=0x85,
  [283]=0xe0, [285]=0x93, [286]=0x84, [288]=0x06, [289]=0xbd,
  [290]=0x78, [291]=0x94, [294]=0xfe, [295]=0xcf,
  [296]=0x53, [297]=0x95, [298]=0x55, [299]=0xb9,
  [300]=0x0f, [301]=0xef, [303]=0x93, [304]=0x85,
  [307]=0xe0, [309]=0x93, [310]=0x84, [312]=0x18, [313]=0x95,
  [314]=0x63, [315]=0x95, [316]=0x65, [317]=0xb9,
  [318]=0x18, [319]=0x95
};

int main(void) {
    avr_t *avr = avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr)) return 2;
    avr->frequency = 16000000;
    avr_loadcode(avr, (uint8_t *)firmware, sizeof firmware, 0);
    unsigned irq_total = 0, irq_timer0 = 0, irq_timer1 = 0;
    for (unsigned step = 1; step <= 1200; ++step) {
        const int status = avr_run(avr);
        if (status != cpu_Running) {
            fprintf(stderr, "WEIGHTOS_IRQ_TRACE_ERROR step=%u status=%d\n",step,status);
            avr_terminate(avr); return 3;
        }
        if (avr->pc == 0x34 || avr->pc == 0x40) {
            unsigned vector = (unsigned)(avr->pc >> 1);
            unsigned saved_pc = (unsigned)avr->data[0x8ff] |
                                ((unsigned)avr->data[0x8fe] << 8);
            unsigned sp = (unsigned)avr->data[0x5d] |
                          ((unsigned)avr->data[0x5e] << 8);
            printf("WEIGHTOS_IRQ_EVENT step=%u vector=%u cycles=%llu saved_pc=%u "
                   "sp=%u portb=%u r21=%u r22=%u\n",
                   step, vector,(unsigned long long)avr->cycle,saved_pc,sp,
                   (unsigned)avr->data[0x25],
                   (unsigned)avr->data[21],(unsigned)avr->data[22]);
            ++irq_total;
            if (vector == 26) ++irq_timer1;
            if (vector == 32) ++irq_timer0;
        }
    }
    printf("WEIGHTOS_IRQ_SUMMARY total=%u timer0=%u timer1=%u\n",
          irq_total,irq_timer0,irq_timer1);
    avr_terminate(avr);
    /* The first pass is observational. Architectural/timing acceptance
       is deliberately decided after comparison with v1.26 traces. */
    return irq_total == 13 && irq_timer0 == 7 && irq_timer1 == 6 ? 0 : 4;
}
