/* ATmega328P TSM regression: update both counters during suspension. */
.text
.global start
start:
    rjmp setup
.org 0x100
setup:
    ldi r16, 2
    out 0x25, r16        /* TCCR0B /8 */
    sts 0x0081, r16      /* TCCR1B /8 */
    ldi r16, 0x81
    out 0x23, r16        /* TSM | PSRSYNC */
    ldi r16, 0x20
    out 0x26, r16        /* TCNT0 = 32 while frozen */
    ldi r16, 0
    sts 0x0085, r16      /* TCNT1H = 0 */
    ldi r16, 0x30
    sts 0x0084, r16      /* TCNT1L = 48 */
    in r19, 0x26         /* first Timer0 */
    lds r20, 0x0084      /* first Timer1 */
    .rept 80
        nop
    .endr
    in r21, 0x26         /* Timer0 after hold */
    lds r22, 0x0084      /* Timer1 after hold */
    ldi r16, 0
    out 0x23, r16        /* release */
    .rept 40
        nop
    .endr
    in r23, 0x26         /* resumed Timer0 */
    lds r24, 0x0084      /* resumed Timer1 */
    ldi r16, 0xa5
    sts 0x0100, r16
finished:
    nop
    rjmp finished
