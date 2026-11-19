/* ATmega328P GTCCR.TSM suspend-and-resume regression firmware. */
.text
.global start
start:
    rjmp setup
.org 0x100
setup:
    ldi r16,2
    out 0x25,r16       /* Timer0 /8 */
    sts 0x0081,r16    /* Timer1 /8 */
    .rept 32
        nop
    .endr
    ldi r16,0x81
    out 0x23,r16       /* TSM|PSRSYNC */
    in r22,0x26        /* Timer0 before hold */
    lds r23,0x0084    /* Timer1 before hold */
    .rept 80
        nop
    .endr
    in r24,0x26        /* Timer0 during hold */
    lds r25,0x0084    /* Timer1 during hold */
    ldi r16,0
    out 0x23,r16       /* release TSM */
    .rept 32
        nop
    .endr
    in r26,0x26        /* Timer0 after release */
    lds r27,0x0084    /* Timer1 after release */
    in r21,0x23        /* GTCCR should be zero */
    ldi r16,0xa5
    sts 0x0100,r16    /* done */
finished:
    nop
    rjmp finished
