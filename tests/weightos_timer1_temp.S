/* Independent ATmega328P Timer1 TEMP-register exercise.
 * Hardware timer stopped throughout (TCCR1B.CS = 0).
 * Datasheet: TCNT1H writes update shared TEMP only; TCNT1L
 * writes commit both bytes, TCNT1L reads latch the actual
 * counter high byte back into TEMP for subsequent TCNT1H read.
 */
.text
.global start
start:
    ldi r16, 0x12
    sts 0x0085, r16     ; TEMP = 0x12, counter not yet changed
    ldi r16, 0x34
    sts 0x0084, r16     ; TCNT1 = 0x1234

    ldi r16, 0x56
    sts 0x0085, r16     ; TEMP = 0x56, actual TCNT1 still 0x1234
    lds r17, 0x0084     ; low = 0x34, TEMP replaced with actual high 0x12
    lds r18, 0x0085     ; high = 0x12

    ldi r16, 0x78
    sts 0x0084, r16     ; TCNT1 = 0x1278 because TEMP was read-latched
    lds r19, 0x0084     ; low = 0x78
    lds r20, 0x0085     ; high = 0x12

    ldi r16, 0xab
    sts 0x0085, r16
    ldi r16, 0xcd
    sts 0x0084, r16     ; TCNT1 = 0xABCD
    lds r21, 0x0084
    lds r22, 0x0085

    ldi r24, 0xa5
    out 0x1e, r24      ; done marker GPIOR0 @ 0x3e
loop:
    nop
    rjmp loop
