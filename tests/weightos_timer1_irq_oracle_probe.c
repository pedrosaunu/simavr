/*
 * WeightOS v1.26 Timer1-only /1 overflow timing diagnostic.
 * Firmware: weightos_timer1_only_v126.S, compiled independently with avr-gcc.
 * The six expected vector cycles and saved PCs come from a pinned,
 * datasheet-derived instruction-boundary scalar schedule, not simavr.
 * This intentionally does NOT assert full MCU or physical timing conformity.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { EXPECTED_EVENTS = 6, IMAGE_CAPACITY = 32768 };
static const unsigned long long expected_cycles[EXPECTED_EVENTS] = {
    278ULL, 548ULL, 818ULL, 1088ULL, 1358ULL, 1628ULL
};
static const unsigned expected_saved_pc[EXPECTED_EVENTS] = {
    0x91U, 0x91U, 0x91U, 0x91U, 0x91U, 0x91U
};

int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[2], "--strict") != 0 &&
                      strcmp(argv[2], "--expect-known-red") != 0 &&
                      strcmp(argv[2], "--observe") != 0)) {
        fprintf(stderr, "usage: %s timer1_only.bin --strict|--expect-known-red\n", argv[0]);
        return 2;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { perror(argv[1]); return 2; }
    uint8_t image[IMAGE_CAPACITY] = {0};
    size_t size = fread(image, 1, sizeof image, fp);
    int read_error = ferror(fp);
    fclose(fp);
    if (read_error || size != 318) {
        fprintf(stderr, "Timer1 fixture must be exactly 318 bytes (got %zu)\n", size);
        return 2;
    }
    avr_t *avr = avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr)) {
        fprintf(stderr, "Unable to initialize ATmega328P\n");
        return 2;
    }
    avr->frequency = 16000000;
    avr_loadcode(avr, image, (uint32_t)size, 0);

    unsigned observed = 0, exact_cycles = 0, exact_pc = 0;
    for (unsigned step = 1; step <= 3000 && observed < EXPECTED_EVENTS; ++step) {
        int status = avr_run(avr);
        if (status != cpu_Running && status != cpu_Sleeping) {
            fprintf(stderr, "MCU stopped at step %u with status %d\n", step, status);
            avr_terminate(avr);
            return 3;
        }
        if (avr->pc != 0x34)
            continue;
        unsigned pc = (unsigned)avr->data[0x8ff] |
                      ((unsigned)avr->data[0x8fe] << 8);
        unsigned long long cyc = (unsigned long long)avr->cycle;
        int cyc_match = cyc == expected_cycles[observed];
        int pc_match = pc == expected_saved_pc[observed];
        exact_cycles += (unsigned)cyc_match;
        exact_pc += (unsigned)pc_match;
        printf("TIMER1_IRQ_BOUNDARY event=%u simavr_cycle=%llu oracle_cycle=%llu "
               "simavr_saved_pc=0x%04x oracle_saved_pc=0x%04x "
               "cycle_equal=%d saved_pc_equal=%d\n",
               observed + 1, cyc, expected_cycles[observed], pc,
               expected_saved_pc[observed], cyc_match, pc_match);
        observed++;
    }
    avr_terminate(avr);
    if (observed != EXPECTED_EVENTS) {
        fprintf(stderr, "Only observed %u/%u Timer1 IRQs\n",
                observed, EXPECTED_EVENTS);
        return 4;
    }
    printf("TIMER1_IRQ_BOUNDARY_SUMMARY events=%u cycle_equal=%u saved_pc_equal=%u\n",
           observed, exact_cycles, exact_pc);
    if (strcmp(argv[2], "--observe") == 0)
        return 0; /* Explicitly observational: never a conformance PASS. */
    if (strcmp(argv[2], "--strict") == 0) {
        /* Only strict physical-clock-compatible architectural fidelity is green. */
        return exact_cycles == EXPECTED_EVENTS && exact_pc == EXPECTED_EVENTS ? 0 : 1;
    }
    /* Stable negative control: currently the candidate prescaler patch
     * does NOT fix the separate /1 IRQ-entry timing discrepancy.
     * When this fails, investigate rather than weakening the strict gate.
     */
    return exact_cycles == 0 && exact_pc == 3 ? 0 : 5;
}
