/* Guest AVR TCNT writes must be allowed while GTCCR TSM|PSRSYNC holds
 * Timer0 and Timer1. See atmega328p_tsm_configure.S for disassembly.
 */
#include "tests.h"
#include "sim_avr.h"
#include <stdint.h>

static const uint8_t firmware[556] = {
    [0] = 0x7f,
    [1] = 0xc0,
    [256] = 0x02,
    [257] = 0xe0,
    [258] = 0x05,
    [259] = 0xbd,
    [261] = 0x93,
    [262] = 0x81,
    [264] = 0x01,
    [265] = 0xe8,
    [266] = 0x03,
    [267] = 0xbd,
    [269] = 0xe2,
    [270] = 0x06,
    [271] = 0xbd,
    [273] = 0xe0,
    [275] = 0x93,
    [276] = 0x85,
    [279] = 0xe3,
    [281] = 0x93,
    [282] = 0x84,
    [284] = 0x36,
    [285] = 0xb5,
    [286] = 0x40,
    [287] = 0x91,
    [288] = 0x84,
    [450] = 0x56,
    [451] = 0xb5,
    [452] = 0x60,
    [453] = 0x91,
    [454] = 0x84,
    [457] = 0xe0,
    [458] = 0x03,
    [459] = 0xbd,
    [540] = 0x76,
    [541] = 0xb5,
    [542] = 0x80,
    [543] = 0x91,
    [544] = 0x84,
    [546] = 0x05,
    [547] = 0xea,
    [549] = 0x93,
    [551] = 0x01,
    [554] = 0xfe,
    [555] = 0xcf,
};

int main(int argc, char **argv)
{
    tests_init(argc, argv);
    avr_t *avr=avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr)) fail("Cannot initialize atmega328p");
    avr->frequency=16000000;
    avr_loadcode(avr, (uint8_t *)firmware, sizeof(firmware), 0);
    for (unsigned step=0; step<2500; ++step) {
        int status=avr_run(avr);
        if (avr->data[0x100]==0xa5) break;
        if (status!=cpu_Running && status!=cpu_Sleeping)
            fail("Guest halted before completion, step=%u state=%d",step,status);
    }
    if (avr->data[0x100]!=0xa5) fail("Guest did not signal completion");
    if (avr->data[19]!=0x20 || avr->data[20]!=0x30)
        fail("Writes during TSM not visible: Timer0=%u Timer1=%u",
            avr->data[19],avr->data[20]);
    if (avr->data[21]!=avr->data[19] || avr->data[22]!=avr->data[20])
        fail("Counters advanced while TSM held: Timer0 %u->%u Timer1 %u->%u",
             avr->data[19],avr->data[21],avr->data[20],avr->data[22]);
    if (avr->data[23]<=avr->data[21] || avr->data[24]<=avr->data[22])
        fail("Counters did not resume after release: Timer0 %u->%u Timer1 %u->%u",
             avr->data[21],avr->data[23],avr->data[22],avr->data[24]);
    avr_terminate(avr);
    tests_success();
    return 0;
}
