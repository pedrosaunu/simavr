/* NU-021F: live Timer1 overflow followed by compare IRQs, repeated.
 * Three mask-specific AVR binaries (TOIE1, TOIE1|OCIE1A, all three).
 * No host-written flags, interrupt PC or CPU state.
 *
 * Timeline:
 *  1. OCR1A/B=0x8000, start CS1=1, reload TCNT1=0xfffe.
 *  2. Hardware overflow sets TOV1. OVF ISR increments r23.
 *  3. In mask=3/7, OVF ISR sets OCR1A=0x0030, OCR1B=0x0035
 *     and resets TCNT1=0, producing two guest-clock compare events.
 *  4. Final enabled compare ISR reloads near overflow, repeats.
 *
 * Software reference only, NOT independent silicon edge timing.
 */
#ifndef IRQ_MASK
#define IRQ_MASK 7
#endif
.text
.global start
start:
    rjmp init
.org 0x2c
    rjmp compa_isr
.org 0x30
    rjmp compb_isr
.org 0x34
    rjmp ovf_isr
.org 0x60
init:
    ldi r16,0x80
    sts 0x89,r16          /* OCR1AH -> TEMP */
    sts 0x8b,r16          /* OCR1BH -> TEMP */
    ldi r16,0
    sts 0x88,r16          /* OCR1A=0x8000 */
    sts 0x8a,r16          /* OCR1B=0x8000 */
    ldi r16,IRQ_MASK
    sts 0x6f,r16          /* TIMSK1 */
    ldi r16,1
    sts 0x81,r16          /* TCCR1B, WGM0, /1 */
    ldi r16,0xff
    sts 0x85,r16          /* TCNT1H */
    ldi r16,0xfe
    sts 0x84,r16          /* TCNT1L commit */
    sei
loop:
    nop
    rjmp loop
.org 0xb0
compa_isr:
    inc r21
#if IRQ_MASK == 3
    rcall reload_overflow
#endif
    reti
compb_isr:
    inc r22
#if IRQ_MASK == 7
    rcall reload_overflow
#endif
    reti
ovf_isr:
    inc r23
#if IRQ_MASK == 1
    rcall reload_overflow
#else
    ldi r16,0
    sts 0x89,r16          /* OCR1AH */
    ldi r16,0x30
    sts 0x88,r16          /* OCR1AL => 0x0030 */
    ldi r16,0
    sts 0x8b,r16          /* OCR1BH */
    ldi r16,0x35
    sts 0x8a,r16          /* OCR1BL => 0x0035 */
    ldi r16,0
    sts 0x85,r16          /* TCNT1H */
    sts 0x84,r16          /* TCNT1L=0 */
#endif
    reti
reload_overflow:
    ldi r16,0xff
    sts 0x85,r16
    ldi r16,0xfe
    sts 0x84,r16
    ret
