/* Regression for ATmega328P GTCCR TSM+PSRSYNC hold semantics.
 *
 * Firmware source: tests/atmega328p_tsm_prescaler.S. The .text bytes
 * are embedded so the host test does not require an AVR compiler.
 * Guest register values come from actual AVR IN/LDS instructions, not
 * avr->data[TCNT0] (which is not necessarily a live timer observation).
 */
#include "tests.h"
#include "sim_avr.h"
#include <stdint.h>

#define SRAM_DONE 0x100
#define DONE 0xa5

static uint8_t firmware[516] = {
    [0] = 0x7f,
    [1] = 0xc0,
    [256] = 0x02,
    [257] = 0xe0,
    [258] = 0x05,
    [259] = 0xbd,
    [261] = 0x93,
    [262] = 0x81,
    [328] = 0x01,
    [329] = 0xe8,
    [330] = 0x03,
    [331] = 0xbd,
    [332] = 0x63,
    [333] = 0xb5,
    [334] = 0x76,
    [335] = 0xb5,
    [336] = 0x80,
    [337] = 0x91,
    [338] = 0x84,
    [500] = 0x96,
    [501] = 0xb5,
    [502] = 0xa0,
    [503] = 0x91,
    [504] = 0x84,
    [506] = 0x05,
    [507] = 0xea,
    [509] = 0x93,
    [511] = 0x01,
    [514] = 0xfe,
    [515] = 0xcf,
};

int main(int argc, char **argv)
{
    avr_t *avr;
    unsigned step;
    tests_init(argc, argv);
    avr = avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr))
        fail("Could not initialize ATmega328P");
    avr->frequency = 16000000;
    avr_loadcode(avr, firmware, sizeof(firmware), 0);
    for (step = 0; step < 2000; ++step) {
        int state = avr_run(avr);
        if (avr->data[SRAM_DONE] == DONE)
            break;
        if (state != cpu_Running && state != cpu_Sleeping)
            fail("Simulator stopped at step %u, state=%d PC=0x%x",
                 step, state, (unsigned)avr->pc);
    }
    if (avr->data[SRAM_DONE] != DONE)
        fail("AVR fixture did not complete within 2000 steps");
    if ((avr->data[22] & 0x81) != 0x81)
        fail("GTCCR.TSM|PSRSYNC not held: r22=0x%02x", avr->data[22]);
    if (avr->data[23] != avr->data[25])
        fail("Timer0 counted during TSM: first=%u, second=%u",
             avr->data[23], avr->data[25]);
    if (avr->data[24] != avr->data[26])
        fail("Timer1 counted during TSM: first=%u, second=%u",
             avr->data[24], avr->data[26]);
    avr_terminate(avr);
    tests_success();
    return 0;
}
