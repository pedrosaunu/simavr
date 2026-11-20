/* Timer1 WGM0 both OCF1A/B pending, stop clock before W1C.
 * Separate AVR variants: OUT precise bit-mask; SBI read-modify-write.
 * Only actual guest instructions generate/clear the flags, with
 * no host vector injection or register side effects.
 */
#ifndef CLEAR_STYLE
#define CLEAR_STYLE 0
#endif
.text
.global start
start:
 rjmp init
.org 0x40
init:
 ldi r16,1
 sts 0x0081,r16      /* WGM0, CS1=1 */
 nop
 nop
 ldi r16,0
 sts 0x0081,r16      /* stop Timer1; retain pending OCF1A/B */
 in r20,0x16        /* TIFR1 before clear */
#if CLEAR_STYLE==0
 ldi r17,0x02
 out 0x16,r17       /* W1C OCF1A only */
#elif CLEAR_STYLE==1
 sbi 0x16,1         /* read-modify-write W1C interaction */
#else
#error Unsupported_CLEAR_STYLE
#endif
 in r21,0x16        /* after first clear */
 ldi r17,0
 out 0x16,r17       /* zero write must preserve remaining flags */
 in r22,0x16
 ldi r17,0x04
 out 0x16,r17       /* clear OCF1B via literal mask */
 in r23,0x16
loop:
 nop
 rjmp loop
