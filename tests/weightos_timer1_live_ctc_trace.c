/* Guest-instruction-boundary trace of real simavr fork executing live CTC.
 * The register snapshot only uses general-purpose guest register values.
 * Raw avr->data[TCNT1L/H] is NEVER an oracle. GPRs only hold values
 * obtained through actual AVR LDS instructions.
 */
#include "sim_avr.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[3], "--trace") != 0)) {
        fprintf(stderr, "usage: %s guest.bin 4|12 --trace\n", argv[0]);
        return 2;
    }
    const int mode = atoi(argv[2]);
    if (mode != 4 && mode != 12) return 2;
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) return 2;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return 2; }
    long len = ftell(fp);
    if (len < 16 || len > 512 || (len % 2) || fseek(fp,0,SEEK_SET) != 0) {
        fclose(fp); return 2;
    }
    uint8_t *firmware = malloc((size_t)len);
    if (!firmware) { fclose(fp); return 2; }
    int read_ok = fread(firmware,1,(size_t)len,fp) == (size_t)len;
    fclose(fp);
    if (!read_ok) { free(firmware); return 2; }
    avr_t *avr = avr_make_mcu_by_name("atmega328p");
    if (!avr || avr_init(avr)) { free(firmware); return 2; }
    avr->frequency=16000000;
    avr_loadcode(avr, firmware, (uint32_t)len, 0);
    free(firmware);
    /* 20 instructions before BREAK; 8 config, 12 observation. */
    const int count=20;
    for (int step=1; step<=count; step++) {
        int status=avr_run(avr);
        if (status!=cpu_Running) {
            fprintf(stderr,"CTC_SIMAVR_UNEXPECTED_EXIT step=%d code=%d\n",step,status);
            avr_terminate(avr);
            return 1;
        }
        printf("CTC_SIMAVR_TRACE wgm=%d step=%d pc=%u cycles=%" PRIu64
               " r17=%u r18=%u r19=%u r20=%u r21=%u r22=%u r23=%u r24=%u\n",
               mode, step, (unsigned)(avr->pc/2), (uint64_t)avr->cycle,
               avr->data[17],avr->data[18],avr->data[19],avr->data[20],
               avr->data[21],avr->data[22],avr->data[23],avr->data[24]);
    }
    printf("CTC_SIMAVR_COMPLETE wgm=%d steps=20 pc=%u cycles=%" PRIu64 "\n",
           mode,(unsigned)(avr->pc/2),(uint64_t)avr->cycle);
    avr_terminate(avr);
    return 0;
}
