/* PSRSYNC reset is not allowed to erase TCNT register values while stopped. */
.text
.global start
start:
    rjmp setup
.org 0x100
setup:
    ldi r16,0x24
    out 0x26,r16       /* TCNT0 while CS0=0 */
    ldi r16,0
    sts 0x0085,r16     /* TCNT1H */
    ldi r16,0x42
    sts 0x0084,r16     /* TCNT1L while CS1=0 */
    ldi r16,1
    out 0x23,r16       /* GTCCR PSRSYNC strobe, no TSM */
    in r19,0x23        /* expected 0: strobe self-cleared */
    in r20,0x26        /* expected 0x24 */
    lds r21,0x0084     /* expected 0x42 */
    .rept 18
        nop
    .endr
    in r22,0x26
    lds r23,0x0084
    ldi r16,0xa5
    sts 0x0100,r16
finished:
    nop
    rjmp finished
