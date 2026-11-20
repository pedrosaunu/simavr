/* Timer0/Timer1 shared prescaler: TSM+PSRSYNC held startup and release.
 * Two separate firmware images differ by exactly eight CPU NOP cycles
 * after TSM release, but otherwise run identical guest register traffic.
 * All observations are actual AVR IN/LDS reads stored in guest SRAM.
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
    ldi r16,0x81
    out 0x23,r16      /* GTCCR.TSM | GTCCR.PSRSYNC */
    ldi r16,0x02
    out 0x25,r16      /* TCCR0B CS02..0=010 */
    sts 0x81,r16      /* TCCR1B CS12..0=010 */
    clr r17
    out 0x26,r17      /* TCNT0=0 */
    sts 0x85,r17      /* TCNT1H=0 */
    sts 0x84,r17      /* TCNT1L=0 */

    in r18,0x26
    lds r19,0x84
    sts 0x110,r18     /* first held Timer0 */
    sts 0x111,r19     /* first held Timer1 */
    .rept 24
       nop
    .endr
    in r18,0x26
    lds r19,0x84
    in r21,0x23       /* read GTCCR under TSM */
    sts 0x112,r18
    sts 0x113,r19
    sts 0x114,r21
    out 0x23,r17      /* release TSM and PSRSYNC */
    .rept NOP_COUNT
       nop
    .endr
    in r18,0x26
    lds r19,0x84
    sts 0x115,r18
    sts 0x116,r19
    .rept 8
       nop
    .endr
    in r18,0x26
    lds r19,0x84
    sts 0x117,r18
    sts 0x118,r19
    ldi r20,0xa5
    sts 0x100,r20
stop:
    rjmp stop
