/* Stopped synchronous timers retain guest TCNT after a PSRSYNC strobe. */
#include "tests.h"
#include "sim_avr.h"
#include <stdint.h>
static const uint8_t firmware[336] = {
    [0] = 0x7f,
    [1] = 0xc0,
    [256] = 0x04,
    [257] = 0xe2,
    [258] = 0x06,
    [259] = 0xbd,
    [261] = 0xe0,
    [263] = 0x93,
    [264] = 0x85,
    [266] = 0x02,
    [267] = 0xe4,
    [269] = 0x93,
    [270] = 0x84,
    [272] = 0x01,
    [273] = 0xe0,
    [274] = 0x03,
    [275] = 0xbd,
    [276] = 0x33,
    [277] = 0xb5,
    [278] = 0x46,
    [279] = 0xb5,
    [280] = 0x50,
    [281] = 0x91,
    [282] = 0x84,
    [320] = 0x66,
    [321] = 0xb5,
    [322] = 0x70,
    [323] = 0x91,
    [324] = 0x84,
    [326] = 0x05,
    [327] = 0xea,
    [329] = 0x93,
    [331] = 0x01,
    [334] = 0xfe,
    [335] = 0xcf,
};
int main(int argc, char **argv) {
    tests_init(argc, argv);
    avr_t *avr=avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr)) fail("Could not initialize MCU");
    avr->frequency=16000000;
    avr_loadcode(avr,(uint8_t *)firmware,sizeof(firmware),0);
    for (unsigned step=0;step<2000;++step) {
        int status=avr_run(avr);
        if (avr->data[0x100]==0xa5) break;
        if (status!=cpu_Running && status!=cpu_Sleeping) fail("MCU stopped early");
    }
    if (avr->data[0x100]!=0xa5) fail("Fixture did not finish");
    if (avr->data[19]!=0) fail("PSRSYNC did not self-clear: %u",avr->data[19]);
    if (avr->data[20]!=0x24 || avr->data[22]!=0x24)
        fail("Stopped TCNT0 lost after PSRSYNC: %u/%u",avr->data[20],avr->data[22]);
    if (avr->data[21]!=0x42 || avr->data[23]!=0x42)
        fail("Stopped TCNT1 lost after PSRSYNC: %u/%u",avr->data[21],avr->data[23]);
    avr_terminate(avr);
    tests_success();
    return 0;
}
