/* Cross-device, independent opcode-cycle table for AVR mapped I/O writes.
 * Checks OUT, STS, indirect X/Y/Z, SBI/CBI and displacement stores.
 * These static expected clock edges come from the instruction schedules
 * in weightos_io_retirement.S, NOT from the simulator's own output.
 */
#include "sim_avr.h"
#include "sim_io.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { IO_ADDR = 0x3e, EVENTS = 9 };
static const unsigned long long expected_cycles[EVENTS] = {
    2, 4, 8, 12, 16, 18, 20, 23, 26
};
static const uint8_t expected_values[EVENTS] = {
    0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5b, 0x5a, 0x5a
};
typedef struct {
    unsigned count;
    unsigned bad;
} observer_t;

static void observe_write(avr_t *avr, avr_io_addr_t addr,
                          uint8_t value, void *param)
{
    observer_t *o = (observer_t *)param;
    const unsigned i = o->count++;
    const int equal = i < EVENTS && addr == IO_ADDR &&
            avr->cycle == expected_cycles[i] &&
            value == expected_values[i];
    printf("IO_RETIRE_EVENT index=%u addr=0x%04x cycle=%llu "
           "expected_cycle=%llu value=0x%02x expected_value=0x%02x equal=%d\n",
           i + 1U, (unsigned)addr,
           (unsigned long long)avr->cycle,
           i < EVENTS ? expected_cycles[i] : 0ULL,
           (unsigned)value,
           i < EVENTS ? (unsigned)expected_values[i] : 0U,
           equal);
    if (!equal) o->bad++;
    avr_core_watch_write(avr, addr, value);
}

static int check_one(const char *mcu, const uint8_t *fw, size_t size)
{
    observer_t o = {0};
    avr_t *a = avr_make_mcu_by_name(mcu);
    if (!a || avr_init(a)) {
        fprintf(stderr, "IO_RETIRE_ERROR mcu=%s init\n", mcu);
        return 1;
    }
    a->frequency = 16000000;
    avr_register_io_write(a, IO_ADDR, observe_write, &o);
    avr_loadcode(a, (uint8_t *)fw, (uint32_t)size, 0);
    for (unsigned step = 0; step < 300 && o.count < EVENTS; ++step) {
        int status = avr_run(a);
        if (status != cpu_Running && o.count < EVENTS) break;
    }
    printf("IO_RETIRE_SUMMARY mcu=%s events=%u mismatches=%u\n",
           mcu, o.count, o.bad);
    const int pass = o.count == EVENTS && o.bad == 0;
    avr_terminate(a);
    return pass ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr,"usage: %s firmware.bin\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 2; }
    long n = ftell(f);
    if (n <= 0 || n > 1024 || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return 2;
    }
    uint8_t *fw = (uint8_t *)malloc((size_t)n);
    if (!fw) { fclose(f); return 2; }
    size_t actual = fread(fw, 1, (size_t)n, f);
    fclose(f);
    if (actual != (size_t)n) { free(fw); return 2; }
    const int a = check_one("atmega328p",fw,(size_t)n);
    const int b = check_one("atmega2560",fw,(size_t)n);
    free(fw);
    printf("IO_RETIRE_CROSS_DEVICE_RESULT=%s\n", (!a && !b) ? "PASS" : "FAIL");
    return a || b;
}
