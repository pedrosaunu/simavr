/* NU-019D running Timer1 ICP1 rising edges, repeated captures and guest W1C.
 * Real AVR firmware, WGM0, ICNC1=0, CS1=1. The separate C harness drives
 * PB0 only at retired guest instruction boundaries; never ICR, TCNT or flags.
 */
#ifndef NU019D_PAD
#define NU019D_PAD 0
#endif
.text
.global start
start:
  rjmp init
.org 0x40
init:
  ldi r16,0
  sts 0x80,r16     /* TCCR1A WGM0 */
  ldi r16,0x41
  sts 0x81,r16     /* ICES1 rising, CS1=1, counter running */
  ldi r17,0x20
  out 0x16,r17     /* acknowledge any preexisting ICF1 */
  ldi r16,0xa5
  sts 0x0100,r16   /* experiment ARMED */
  .rept 5+NU019D_PAD
    nop
  .endr
  lds r18,0x86    /* ICR1L, snapshots high into TEMP */
  lds r19,0x87    /* ICR1H, must use same TEMP */
  in r20,0x16     /* TIFR1 before W1C */
  out 0x16,r17    /* W1C ICF1 between possible external captures */
  .rept 7
    nop
  .endr
  lds r21,0x86    /* ICR1L after later captures */
  lds r22,0x87    /* ICR1H */
  in r23,0x16     /* second TIFR1 read */
  lds r24,0x84    /* guest-visible running TCNT1L */
  lds r25,0x85    /* high byte through TEMP */
  ldi r16,0x5a
  sts 0x0101,r16  /* experiment DONE */
final_loop:
  nop
  rjmp final_loop
