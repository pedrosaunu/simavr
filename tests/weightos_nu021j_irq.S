/* NU-021J: real ATmega328P program pending capture, compare A/B and
 * overflow simultaneously while CLI/TIMSK1=0, then enabling interrupts.
 * Only the PB0 waveform comes from the external test fixture. Three
 * separate builds alter mask after CAPT ISR (firmware writes TIMSK1).
 * No host can seed TIFR1, CPU vectors, SRAM, SREG, PC, or interrupt flags.
 */
#ifndef NU021J_CASE
#define NU021J_CASE 0
#endif
#if NU021J_CASE == 0
#define UNMASK 0x27
#define AFTER_CAPT 0x07
#elif NU021J_CASE == 1
#define UNMASK 0x23
#define AFTER_CAPT 0x03
#elif NU021J_CASE == 2
#define UNMASK 0x25
#define AFTER_CAPT 0x05
#else
#error invalid NU021J_CASE
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
  ldi r16,UNMASK
  sts 0x6f,r16             /* dynamic mask program-controlled */
  sei
loop:
  nop                     /* SEI one-instruction latency */
  rjmp loop
.org 0x150
capture_isr:
  inc r20
  ldi r17,AFTER_CAPT
  sts 0x6f,r17            /* reprogram mask inside actual capture ISR */
  reti
compa_isr:
  inc r21
  reti
compb_isr:
  inc r22
  reti
overflow_isr:
  inc r23
  reti
