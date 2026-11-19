/* Regression: Timer0/Timer1 pause and resume with GTCCR.TSM|PSRSYNC.
 * Assembly reference: atmega328p_tsm_release.S. Firmware is embedded to
 * keep the ordinary host build independent of an AVR assembler.
 */
#include "tests.h"
#include "sim_avr.h"
#include <stdint.h>

static uint8_t firmware[590] = {
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
    [332] = 0x66,
    [333] = 0xb5,
    [334] = 0x70,
    [335] = 0x91,
    [336] = 0x84,
    [498] = 0x86,
    [499] = 0xb5,
    [500] = 0x90,
    [501] = 0x91,
    [502] = 0x84,
    [505] = 0xe0,
    [506] = 0x03,
    [507] = 0xbd,
    [572] = 0xa6,
    [573] = 0xb5,
    [574] = 0xb0,
    [575] = 0x91,
    [576] = 0x84,
    [578] = 0x53,
    [579] = 0xb5,
    [580] = 0x05,
    [581] = 0xea,
    [583] = 0x93,
    [585] = 0x01,
    [588] = 0xfe,
    [589] = 0xcf,
};

int main(int argc, char **argv)
{
    avr_t *avr;
    unsigned step;
    tests_init(argc, argv);
    avr=avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr))
        fail("Could not initialize ATmega328P");
    avr->frequency=16000000;
    avr_loadcode(avr, firmware, sizeof(firmware), 0);
    for (step=0; step<2000; ++step) {
        int state=avr_run(avr);
        if (avr->data[0x100]==0xa5)
            break;
        if (state!=cpu_Running && state!=cpu_Sleeping)
            fail("Stopped early at step %u state=%d",step,state);
    }
    if (avr->data[0x100]!=0xa5)
        fail("AVR fixture timed out");
    if (avr->data[22]!=avr->data[24] || avr->data[23]!=avr->data[25])
        fail("TSM did not freeze counters: Timer0=%u->%u Timer1=%u->%u",
            avr->data[22], avr->data[24], avr->data[23], avr->data[25]);
    if (avr->data[26]==avr->data[24] || avr->data[27]==avr->data[25])
        fail("Timer counters did not resume after TSM release: "
             "Timer0=%u->%u Timer1=%u->%u",
             avr->data[24], avr->data[26], avr->data[25], avr->data[27]);
    if (avr->data[21]!=0)
        fail("GTCCR did not clear after release: r21=%u",avr->data[21]);
    avr_terminate(avr);
    tests_success();
    return 0;
}
