/* NU-021K — boundary-crossing ICP1/TOV1/OCF1A/B experiment.
 * Timer1 runs in WGM0 /1 from TCNT1=0xffe8. OCR1A=0xffff is a
 * pre-rollover compare; OCR1B=0x0000 is a rollover-adjacent compare.
 * The external witness ONLY drives PB0, after retiring a guest AVR NOP.
 * All timer flags and capture registers belong to the simulated chip.
 */
.text
.global start
start:
 rjmp init
.org 0x70
init:
 cli
 ldi r16,0x00
 sts 0x80,r16       /* TCCR1A WGM0 */
 ldi r16,0xff
 sts 0x89,r16       /* OCR1AH */
 sts 0x88,r16       /* OCR1A=ffff */
 ldi r16,0
 sts 0x8b,r16       /* OCR1BH */
 sts 0x8a,r16       /* OCR1B=0000 */
 ldi r16,0xff
 sts 0x85,r16       /* TEMP TCNT1H */
 ldi r16,0xe8
 sts 0x84,r16       /* TCNT1=ffe8 */
 ldi r16,0
 sts 0x6f,r16       /* IRQ masks off */
 ldi r16,0x41
 sts 0x81,r16       /* ICES1 rising; CS=/1; ICNC1=0 */
 ldi r16,0xa5
 sts 0x100,r16      /* external edge observer ready */
 .rept 90
  nop
 .endr
 ldi r16,0x40
 sts 0x81,r16       /* Stop Timer1, keep ICES1 */
 lds r24,0x86      /* Hardware captured ICR1L */
 lds r25,0x87      /* Hardware captured ICR1H */
 in r26,0x16       /* TIFR1 is IO 0x16 */
loop:
 nop
 rjmp loop
