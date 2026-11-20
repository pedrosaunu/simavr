/* Original-simavr guest-instruction probe; no IRQ/flag injection.
 * Event and write cycles are reported by a LOG-ONLY instrument in avr_timer.c.
 * The probe logs every architectural guest instruction boundary; the
 * reference's software event ordering is NOT asserted to equal silicon.
 */
#include "sim_avr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char**argv){
    if(argc!=4)return 2;
    unsigned ocr=(unsigned)strtoul(argv[2],NULL,0);
    unsigned nops=(unsigned)strtoul(argv[3],NULL,0);
    if(ocr>15||nops>12)return 2;
    uint8_t program[32768]={0};
    FILE*f=fopen(argv[1],"rb");
    if(!f){perror(argv[1]);return 2;}
    size_t n=fread(program,1,sizeof(program),f);fclose(f);
    if(n<80||n>=sizeof(program))return 2;
    avr_t*a=avr_make_mcu_by_name("atmega328p");
    if(!a||avr_init(a))return 2;
    a->frequency=16000000;
    avr_loadcode(a,program,(uint32_t)n,0);
    int saw_out=0,saw_after=0,saw_stop=0;
    unsigned cycles=0,steps=0;
    for(unsigned i=0;i<64;i++){
        int rc=avr_run(a);
        if(rc!=cpu_Running&&rc!=cpu_Sleeping){
            fprintf(stderr,"NU019_GUEST_HALTED ocr=%u nops=%u rc=%d\n",ocr,nops,rc);
            avr_terminate(a);return 3;
        }
        const unsigned pc=(unsigned)a->pc/2U;
        cycles=(unsigned)a->cycle;
        steps=i+1;
        if(a->data[20]||saw_out) saw_out=1;
        if(saw_out&&pc>=0x32+nops)saw_after=1;
        if(saw_after&&!a->data[0x81])saw_stop=1;
        printf("NU019_REF_GUEST ocr=%u nops=%u step=%u pc_word=%04x "
               "cycles=%u r19=%02x r20=%02x r21=%02x "
               "tifr1=%02x tccr1b=%02x\n",
               ocr,nops,steps,pc,cycles,
               (unsigned)a->data[19],(unsigned)a->data[20],
               (unsigned)a->data[21],(unsigned)a->data[0x36],
               (unsigned)a->data[0x81]);
        if(saw_stop&&pc>=0x36+nops)break;
    }
    printf("NU019_REF_SUMMARY ocr=%u nops=%u steps=%u cycles=%u "
           "r19_before=%02x r20_after=%02x r21_stopped=%02x "
           "TIFR1=%02x pass=%d\n",
           ocr,nops,steps,cycles,(unsigned)a->data[19],
           (unsigned)a->data[20],(unsigned)a->data[21],
           (unsigned)a->data[0x36],saw_stop);
    avr_terminate(a);
    return saw_stop?0:4;
}
