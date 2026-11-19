/* Read actual guest-visible TCNT0 and TCNT1L via AVR IN/LDS and report
 * both values. The firmware is compiled from weightos_shared_phase.S.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) return 2;
    uint8_t image[32768] = {0};
    size_t n = fread(image, 1, sizeof(image), fp);
    fclose(fp);
    avr_t *a = avr_make_mcu_by_name("atmega328p");
    if (!a || avr_init(a)) return 2;
    a->frequency = 16000000;
    avr_loadcode(a, image, (uint32_t)n, 0);
    for (unsigned i = 0; i < 1000; i++) {
        int s = avr_run(a);
        if (a->data[0x100] == 0xa5) {
            printf("WEIGHTOS_PHASE nop=%s tcnt0=%u tcnt1=%u gtccr=%u cycles=%llu\n",
                argv[2],(unsigned)a->data[19],(unsigned)a->data[20],
                (unsigned)a->data[0x43],(unsigned long long)a->cycle);
            avr_terminate(a);
            return 0;
        }
        if (s != cpu_Running) { avr_terminate(a); return 3; }
    }
    avr_terminate(a);
    return 4;
}
