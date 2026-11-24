/* NU-021L: real guest, no host IRQ or stack injection. Three cases:
 * 0: CAPT -> COMPA -> COMPB nested; OVF after outer return.
 * 1: CAPT -> COMPA nested; OVF after outer return; COMPB stays masked.
 * 2: SEI; CLI blocks nested entry; all service is sequential.
 */
#ifndef NU021L_CASE
#define NU021L_CASE 0
#endif
.text
.global start
start:
  rjmp init
.org 0x28
  rjmp capture_isr
.org 0x2c
  rjmp compa_isr
.org 0x30
  rjmp compb_isr
.org 0x34
  rjmp overflow_isr
.org 0x70
init:
  cli
  ldi r16,0
  sts 0x80,r16             /* TCCR1A WGM0 */
  sts 0x89,r16             /* OCR1AH */
  ldi r16,0x18
  sts 0x88,r16             /* OCR1A=0x18 */
  ldi r16,0
  sts 0x8b,r16             /* OCR1BH */
  ldi r16,0x28
  sts 0x8a,r16             /* OCR1B=0x28 */
  ldi r16,0xff
  sts 0x85,r16             /* TCNT1H staged */
  ldi r16,0xd0
  sts 0x84,r16             /* TCNT1=0xffd0 */
  ldi r16,0
  sts 0x6f,r16             /* all IRQ masks OFF */
  ldi r16,0x41
  sts 0x81,r16             /* ICES1 rising, WGM0, CS1=/1 */
  ldi r16,0xa5
  sts 0x100,r16            /* external PB0 waveform READY */
  .rept 138
    nop                   /* accumulate ICF1, OCF1A/B, TOV1 */
  .endr
  ldi r16,0x20
  sts 0x6f,r16             /* dynamic mask program-controlled */
  sei
loop:
  nop                     /* SEI one-instruction latency */
  rjmp loop
.org 0x250
capture_isr:
  inc r20
  ldi r17,0x02
  sts 0x6f,r17
  sei
#if NU021L_CASE == 2
  cli
#else
  nop
  cli
#endif
  inc r24
  ldi r17,0x01
#if NU021L_CASE == 2
  ldi r17,0x07
#endif
  sts 0x6f,r17
  reti
compa_isr:
  inc r21
#if NU021L_CASE == 0
  ldi r17,0x04
  sts 0x6f,r17
  sei
  nop
  cli
  inc r25
#endif
  reti
compb_isr:
  inc r22
  reti
overflow_isr:
  inc r23
  reti
