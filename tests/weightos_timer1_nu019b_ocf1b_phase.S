/* NU-019B: independently compiled real guest OCF1B/W1C event phase.
 * OCF1A target deliberately placed outside the observation window.
 * NO simulator host flag, timer or register injection.
 */
#ifndef NU019_OCR_B
#define NU019_OCR_B 3
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
 out 0x16,r17             /* clear pending OCF1A/B */
 ldi r16,0xff
 sts 0x0088,r16          /* OCR1AL = 0xff; suppress nearby OCF1A */
 ldi r16,NU019_OCR_B
 sts 0x008a,r16          /* OCR1BL = sweep target */
 ldi r16,0x01
 sts 0x0081,r16          /* normal WGM0, CS1 clock */
 .rept NU019_NOP_COUNT
 nop
 .endr
 in r19,0x16             /* TIFR1 before hardware event / clear */
 ldi r17,0x04
 out 0x16,r17            /* W1C OCF1B, bit 2 */
 in r20,0x16             /* OCF1B after clear */
 ldi r16,0
 sts 0x0081,r16          /* stop Timer1 (retains TCNT1) */
 in r21,0x16
final_loop:
 nop
 rjmp final_loop
