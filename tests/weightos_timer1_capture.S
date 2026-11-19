/* Direct ICP1 positive-edge guest fixture, ATmega328P WGM0, timer stopped.
 * The external harness changes PB0 only after the startup marker.
 * r17/r18/r19 contain guest-visible ICR1L/ICR1H/TIFR1 readbacks.
 */
.global start
.section .text
start:
    ldi r16, 0
    sts 0x80, r16    /* TCCR1A WGM0 */
    sts 0x81, r16    /* TCCR1B stopped */
    sts 0x85, r16    /* TCNT1H = 0x12 */
    ldi r16, 0x12
    sts 0x85, r16
    ldi r16, 0x34
    sts 0x84, r16
    ldi r16, 0x40
    sts 0x81, r16    /* ICES1 rising, ICNC1=0, CS=0 */
    ldi r16, 0xa5
    sts 0x100, r16  /* Ready for external PB0 edge */
    .rept 12
       nop
    .endr
    lds r17, 0x86  /* ICR1L; latches high TEMP */
    lds r18, 0x87  /* ICR1H */
    lds r19, 0x36  /* TIFR1 */
    ldi r16, 0x5a
    sts 0x101, r16  /* Done marker */
done:
    rjmp done
