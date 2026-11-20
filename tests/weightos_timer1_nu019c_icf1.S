/* NU-019C: external ICP1 edge, capture flag and genuine AVR W1C.
 * WGM0, stopped Timer1, ICES1 rising, ICNC1=0. No direct flag injection.
 * A probe drives only PB0 after a guest-authored SRAM readiness marker.
 */
#ifndef NU019C_NOPS
#define NU019C_NOPS 0
#endif
.text
.global start
start:
 rjmp init
.org 0x40
init:
 ldi r16,0
 sts 0x80,r16    /* TCCR1A normal WGM0 */
 sts 0x81,r16    /* TCCR1B stop */
 ldi r16,0x12
 sts 0x85,r16    /* TCNT1H staging register */
 ldi r16,0x34
 sts 0x84,r16    /* TCNT1L commits 0x1234 */
 ldi r16,0x40
 sts 0x81,r16    /* ICES1 rising; ICNC1=0, CS=0 */
 ldi r17,0x20
 out 0x16,r17    /* clear pending capture flag before test */
 ldi r16,0xa5
 sts 0x0100,r16  /* external PB0 experiment armed */
 .rept NU019C_NOPS
 nop
 .endr
 in r19,0x16     /* TIFR1 immediately before W1C */
 out 0x16,r17    /* W1C ICF1 */
 in r20,0x16     /* TIFR1 immediately after W1C */
 lds r21,0x86    /* ICR1L, captured low */
 lds r22,0x87    /* ICR1H, captured high */
 in r23,0x16     /* final TIFR1 */
 ldi r16,0x5a
 sts 0x0101,r16  /* guest done */
final_loop:
 nop
 rjmp final_loop
