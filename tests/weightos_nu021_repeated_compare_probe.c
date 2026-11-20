/* NU-021 reference: repeated guest-generated Timer1 COMPA/COMPB ISR.
 * No host flag/vector seeding. Every traced PC, cycle, SP, return address,
 * GPR and peripheral flag is read from the independently executing AVR.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_STEPS 300

static int run(const char *path, unsigned mask) {
    uint8_t image[32768] = {0};
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 2; }
    size_t n = fread(image, 1, sizeof(image), f);
    fclose(f);
    if (n < 0x60 || n >= sizeof(image)) return 2;
    avr_t *a = avr_make_mcu_by_name("atmega328p");
    if (!a || avr_init(a)) return 2;
    a->frequency = 16000000;
    avr_loadcode(a, image, (uint32_t)n, 0);

    unsigned sp_initial = a->data[0x5d] + 256U * a->data[0x5e];
    unsigned seen_a = 0, seen_b = 0, seen_overflow = 0;
    unsigned count_reti = 0, last_pc = 0xffff, last_sp = sp_initial;
    unsigned complete = 0, steps = 0;
    for (unsigned i = 0; i < MAX_STEPS; ++i) {
        int rc = avr_run(a);
        if (rc != cpu_Running && rc != cpu_Sleeping) {
            fprintf(stderr, "NU021_HALTED mask=%02x rc=%d step=%u\n", mask, rc, i);
            avr_terminate(a);
            return 3;
        }
        unsigned pc = (unsigned)a->pc / 2U;
        unsigned sp = a->data[0x5d] + 256U * a->data[0x5e];
        unsigned saved = (sp_initial < 0x900 && sp_initial >= 0x100)
            ? (unsigned)a->data[sp_initial] + 256U * a->data[sp_initial-1]
            : 0;
        if (pc == 0x16 && last_pc != 0x16) ++seen_a;
        if (pc == 0x18 && last_pc != 0x18) ++seen_b;
        if (pc == 0x1a && last_pc != 0x1a) ++seen_overflow;
        if (last_sp < sp_initial && sp == sp_initial) ++count_reti;
        last_sp = sp;
        steps = i + 1;
        printf("NU021_GUEST_TRACE mask=%02x step=%u pc_word=%04x "
               "cycle=%llu r21=%02x r22=%02x r23=%02x "
               "tifr1=%02x timsk1=%02x I=%u sp_delta=%d saved_pc=%04x\n",
               mask, steps, pc, (unsigned long long)a->cycle,
               (unsigned)a->data[21], (unsigned)a->data[22],
               (unsigned)a->data[23], (unsigned)a->data[0x36],
               (unsigned)a->data[0x6f], (unsigned)a->sreg[S_I],
               (int)sp_initial - (int)sp, saved);
        if (a->data[21] >= ((mask & 2) ? 3U : 0U) &&
            a->data[22] >= ((mask & 4) ? 3U : 0U) &&
            ((!(mask & 2)) || a->data[21] >= 3) &&
            ((!(mask & 4)) || a->data[22] >= 3) &&
            sp == sp_initial) {
            complete = 1;
            break;
        }
        last_pc = pc;
    }
    const int ok = complete &&
        (seen_a >= ((mask & 2) ? 3U : 0U)) &&
        (seen_b >= ((mask & 4) ? 3U : 0U)) &&
        (a->data[21] >= ((mask & 2) ? 3U : 0U)) &&
        (a->data[22] >= ((mask & 4) ? 3U : 0U)) &&
        ((mask & 2) || a->data[21] == 0) &&
        ((mask & 4) || a->data[22] == 0) &&
        seen_overflow == 0 && count_reti >= 2;
    printf("NU021_GUEST_SUMMARY mask=%02x steps=%u compa_entries=%u "
           "compb_entries=%u ovf_entries=%u compa_count=%u "
           "compb_count=%u reti_observed=%u pass=%d\n",
           mask, steps, seen_a, seen_b, seen_overflow,
           (unsigned)a->data[21], (unsigned)a->data[22],count_reti,ok);
    avr_terminate(a);
    return ok ? 0 : 1;
}
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    unsigned mask = (unsigned)strtoul(argv[2], NULL, 0);
    if (mask != 2 && mask != 4 && mask != 6) return 2;
    return run(argv[1], mask);
}
