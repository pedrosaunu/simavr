/* NU-021G: independent Timer1 ICP1 + COMPA/COMPB ISR guest.
 * Real WGM0/CS1 timer match events, physical PB0 input driven only by
 * harness pin waveform. No host writes to ICF1/OCF1, PC, SREG or IRQ.
 * Three binary masks: ICIE1, ICIE1|OCIE1A, ICIE1|OCIE1A|OCIE1B.
 */
#ifndef NU021G_MASK
#define NU021G_MASK 0x26
#endif
.text
.global start
start:
  rjmp init
.org 0x28         /* TIMER1_CAPT word vector 0x0014 */
  rjmp icp_isr
.org 0x2c         /* TIMER1_COMPA word vector 0x0016 */
  rjmp compa_isr
.org 0x30         /* TIMER1_COMPB word vector 0x0018 */
  rjmp compb_isr
.org 0x34
  rjmp ovf_isr
.org 0x70
init:
  ldi r16,0
  sts 0x80,r16   /* WGM0 */
  sts 0x89,r16   /* OCR1AH */
  ldi r16,0x50
  sts 0x88,r16   /* OCR1A=0x0050 */
  ldi r16,0
  sts 0x8b,r16   /* OCR1BH */
  ldi r16,0x60
  sts 0x8a,r16   /* OCR1B=0x0060 */
  ldi r16,0
  sts 0x85,r16
  sts 0x84,r16   /* TCNT1=0 */
  ldi r16,NU021G_MASK
  sts 0x6f,r16   /* TIMSK1 */
  ldi r16,0x41
  sts 0x81,r16   /* ICES1 rising, CS1=1, ICNC1=0 */
  sei
  ldi r16,0xa5
  sts 0x100,r16  /* READY marker for PB0 external waveform */
loop:
  nop
  rjmp loop
.org 0xc0
icp_isr:
  inc r20
  reti
compa_isr:
  inc r21
#if NU021G_MASK == 0x22
  rcall reset_timer
#endif
  reti
compb_isr:
  inc r22
#if NU021G_MASK == 0x26
  rcall reset_timer
#endif
  reti
ovf_isr:
  inc r23
  reti
reset_timer:
  ldi r16,0
  sts 0x85,r16
  sts 0x84,r16
  ret
