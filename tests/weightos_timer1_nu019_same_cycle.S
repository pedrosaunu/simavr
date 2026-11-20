/* NU-019: real guest Timer1 compare (OCF1A) meets software TIFR1 W1C.
 * This file is built in several independent combinations of OCR value and
 * post-start NOP count to obtain a controlled oscillator/CPU phase sweep.
 * Only AVR instructions modify OCR1A, OCR1B, TCCR1B and TIFR1.
 * A simulator-observer records WHEN the hardware flag is set and when the
 * software W1C is written, at equal CPU-cycle numbers if such a case arises.
 */
#ifndef NU019_OCR_A
#define NU019_OCR_A 3
#endif
#ifndef NU019_NOP_COUNT
#define NU019_NOP_COUNT 0
#endif
.text
.global start
start:
 rjmp init
.org 0x40
init:
 ldi r17,0x06
 out 0x16,r17        /* clear prior OCF1A/B */
 ldi r16,NU019_OCR_A
 sts 0x0088,r16      /* OCR1AL */
 ldi r16,0xff
 sts 0x008a,r16      /* OCR1BL; avoid unrelated OCF1B */
 ldi r16,0x01
 sts 0x0081,r16      /* normal WGM0 CS1 running */
 .rept NU019_NOP_COUNT
 nop
 .endr
 in r19,0x16         /* flag state immediately before W1C */
 ldi r17,0x02
 out 0x16,r17        /* try to clear newly generated OCF1A */
 in r20,0x16         /* guest-visible state immediately afterward */
 ldi r16,0
 sts 0x0081,r16      /* stop clock */
 in r21,0x16         /* after stop */
final_loop:
 nop
 rjmp final_loop
