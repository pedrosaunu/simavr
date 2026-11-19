/* WeightOS shared-phase probe. Build with -DNOP_COUNT=3 or 11.
 * Reset prescaler once; start Timer0 and Timer1 a few CPU clocks apart,
 * then sample both counters via genuine guest AVR instructions.
 */
#ifndef NOP_COUNT
#define NOP_COUNT 3
#endif
.text
.global start
start:
 rjmp init
.org 0x100
init:
 ldi r16, 2
 ldi r17, 1
 out 0x23, r17   ; GTCCR.PSRSYNC
 out 0x25, r16   ; Timer0 /8
 nop
 nop
 sts 0x81, r16  ; Timer1 /8
 .rept NOP_COUNT
 nop
 .endr
 in r19, 0x26   ; TCNT0
 lds r20, 0x84  ; TCNT1L
 ldi r21, 0xa5
 sts 0x100, r21
stop:
 rjmp stop
