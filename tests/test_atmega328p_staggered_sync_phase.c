/* ATmega328P Timer0/Timer1 share a continuously-running synchronous divider.
 * Regression with two independent, pinned guest AVR .text images.
 * Firmware: GTCCR.PSRSYNC=1; TCCR0B=/8; 2x NOP; TCCR1B=/8;
 * 3 or 11 more NOPs; guest IN TCNT0; guest LDS TCNT1L; mark completion.
 *
 * Both start operations occur before the same prescaler edge. The old
 * per-timer epoch model returned 1/0 and 2/1, respectively. The shared
 * phase model returns 1/1 and 2/2 and self-clears PSRSYNC.
 *
 * The binaries were assembled from tests/weightos_shared_phase.S
 * (NO external AVR toolchain required to run this native host test).
 */
#include "tests.h"
#include "sim_avr.h"
#include <stdint.h>
#include <stddef.h>

static const uint8_t image3[292] = {
  [0]=0x7f,[1]=0xc0,[256]=0x02,[257]=0xe0,[258]=0x11,[259]=0xe0,
  [260]=0x13,[261]=0xbd,[262]=0x05,[263]=0xbd,[269]=0x93,[270]=0x81,
  [278]=0x36,[279]=0xb5,[280]=0x40,[281]=0x91,[282]=0x84,
  [284]=0x55,[285]=0xea,[286]=0x50,[287]=0x93,[289]=0x01,
  [290]=0xff,[291]=0xcf
};
static const uint8_t image11[308] = {
  [0]=0x7f,[1]=0xc0,[256]=0x02,[257]=0xe0,[258]=0x11,[259]=0xe0,
  [260]=0x13,[261]=0xbd,[262]=0x05,[263]=0xbd,[269]=0x93,[270]=0x81,
  [294]=0x36,[295]=0xb5,[296]=0x40,[297]=0x91,[298]=0x84,
  [300]=0x55,[301]=0xea,[302]=0x50,[303]=0x93,[305]=0x01,
  [306]=0xff,[307]=0xcf
};

static void check(const uint8_t *program, size_t size, unsigned expected)
{
    avr_t *avr = avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr))
        fail("Cannot initialize ATmega328P");
    avr->frequency = 16000000;
    avr_loadcode(avr, (uint8_t *)program, (uint32_t)size, 0);
    unsigned step;
    for (step = 0; step < 100; ++step) {
        int state = avr_run(avr);
        if (avr->data[0x100] == 0xa5) break;
        if (state != cpu_Running)
            fail("Guest execution failed: step=%u status=%d",step,state);
    }
    if (avr->data[0x100] != 0xa5)
        fail("Staggered-clock guest timed out");
    if (avr->data[0x43] != 0)
        fail("GTCCR.PSRSYNC must clear: got 0x%02x", avr->data[0x43]);
    if (avr->data[19] != expected || avr->data[20] != expected)
        fail("Shared prescaler mismatch: expected %u, Timer0=%u Timer1=%u",
             expected, avr->data[19], avr->data[20]);
    avr_terminate(avr);
}
int main(int argc, char **argv)
{
    tests_init(argc, argv);
    check(image3, sizeof(image3), 1);
    check(image11, sizeof(image11), 2);
    tests_success();
    return 0;
}
