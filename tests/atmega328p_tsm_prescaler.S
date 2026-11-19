/*
 * ATmega328P TSM|PSRSYNC test firmware. The native host test embeds its
 * assembled .text bytes, so no AVR toolchain is needed to run make t.
 */
.text
.global start
start:
    rjmp setup
.org 0x100
setup:
    ldi r16, 2
    out 0x25, r16       /* TCCR0B clk/8 */
    sts 0x0081, r16    /* TCCR1B clk/8 */
    .rept 32
        nop
    .endr
    ldi r16, 0x81
    out 0x23, r16       /* GTCCR.TSM | GTCCR.PSRSYNC */
    in r22, 0x23        /* GTCCR hold readback */
    in r23, 0x26        /* first TCNT0 sample */
    lds r24, 0x0084    /* first TCNT1L sample */
    .rept 80
        nop
    .endr
    in r25, 0x26        /* second TCNT0 sample */
    lds r26, 0x0084    /* second TCNT1L sample */
    ldi r16, 0xA5
    sts 0x0100, r16    /* signal completion in SRAM */
finished:
    nop
    rjmp finished
