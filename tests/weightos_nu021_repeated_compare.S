/* NU-021 independent repeated Timer1 COMPA/COMPB service from reset.
 * All interrupt flags are produced by running WGM0/CS1 Timer1 in simavr:
 * the host never writes TIFR1, SREG.I or CPU registers for a desired IRQ.
 * Mask variants are independent compiled AVR firmware binaries.
 *
 * This is a software-reference probe, NOT physical same-edge qualification.
 */
#ifndef IRQ_MASK
#define IRQ_MASK 6
#endif
.text
.global start
start:
    rjmp init
.org 0x2c                  /* TIMER1_COMPA word vector 0x0016 */
    rjmp compa_isr
.org 0x30                  /* TIMER1_COMPB word vector 0x0018 */
    rjmp compb_isr
.org 0x34                  /* TIMER1_OVF word vector 0x001a */
    rjmp ovf_isr
.org 0x60
init:
    ldi r16,0x00
    sts 0x89,r16           /* OCR1AH high-byte TEMP */
    ldi r16,0x04
    sts 0x88,r16           /* OCR1AL commits 0x0004 */
    ldi r16,0x00
    sts 0x8b,r16           /* OCR1BH high-byte TEMP */
    ldi r16,0x07
    sts 0x8a,r16           /* OCR1BL commits 0x0007 */
    ldi r16,0x00
    sts 0x85,r16           /* TCNT1H high-byte TEMP */
    ldi r16,0x00
    sts 0x84,r16           /* TCNT1L commits 0x0000 */
    ldi r16,IRQ_MASK
    sts 0x6f,r16           /* TIMSK1 mask */
    ldi r16,1
    sts 0x81,r16           /* TCCR1B CS1 /1, WGM0 */
    sei
loop:
    nop
    rjmp loop
.org 0xa0
compa_isr:
    inc r21
#if IRQ_MASK == 2
    rcall reload_timer
#endif
    reti
compb_isr:
    inc r22
    rcall reload_timer
    reti
ovf_isr:
    inc r23
    reti
reload_timer:
    ldi r16,0x00
    sts 0x85,r16
    ldi r16,0x00
    sts 0x84,r16
    ret
