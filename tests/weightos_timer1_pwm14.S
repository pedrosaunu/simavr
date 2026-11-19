/* Independent AVR guest firmware for mode-14 Fast PWM /1.
 * Programs ICR1=3, OCR1A=1, OCR1B=2 and DDRB PB1/PB2 outputs.
 * SRAM marker identifies completion of startup. Timer hardware owns timing.
 */
.global start
.section .text
start:
    ldi r16, 0x06
    out 0x04, r16             /* DDRB PB1/PB2 outputs */
    ldi r16, 0x00
    sts 0x81, r16            /* TCCR1B stop */
    sts 0x85, r16            /* TCNT1H */
    sts 0x84, r16            /* TCNT1L */
    sts 0x87, r16            /* ICR1H */
    ldi r16, 0x03
    sts 0x86, r16            /* ICR1L TOP */
    ldi r16, 0x00
    sts 0x89, r16            /* OCR1AH */
    sts 0x8b, r16            /* OCR1BH */
    ldi r16, 0x01
    sts 0x88, r16            /* OCR1AL */
    ldi r16, 0x02
    sts 0x8a, r16            /* OCR1BL */
    ldi r16, 0xa2
    sts 0x80, r16            /* TCCR1A COM1A/B=10 WGM11 */
    ldi r16, 0x19
    sts 0x81, r16            /* TCCR1B WGM14 /1 */
    ldi r16, 0xa5
    sts 0x100, r16           /* SRAM program-ready marker */
    .rept 96
        nop
    .endr
forever:
    rjmp forever
