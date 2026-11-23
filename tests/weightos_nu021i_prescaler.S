/* NU-021I: real AVR firmware probes TCNT1 write suppression when Timer1
 * stopped then started, and with shared /8 prescaler phase. Twelve binaries.
 * No host-triggered IRQ, register modifications, or synthetic CPU states.
 * Datasheet ATmega328P sec 15.7.2 and shared sync prescaler chapter 16.
 */
#ifndef NU021I_CASE
#define NU021I_CASE 0
#endif
#if NU021I_CASE < 0 || NU021I_CASE > 11
#error invalid NU021I_CASE
#endif
#if NU021I_CASE == 0 || NU021I_CASE == 1
#define TIMER_CS 1
#else
#define TIMER_CS 2
#endif
#if NU021I_CASE >= 8
#define START_RUNNING 1
#else
#define START_RUNNING 0
#endif
#if NU021I_CASE == 1 || NU021I_CASE == 4 || NU021I_CASE == 9
#define DELAY_NOPS 3
#elif NU021I_CASE == 3
#define DELAY_NOPS 1
#elif NU021I_CASE == 5 || NU021I_CASE == 10
#define DELAY_NOPS 7
#elif NU021I_CASE == 6
#define DELAY_NOPS 4
#elif NU021I_CASE == 7 || NU021I_CASE == 11
#define DELAY_NOPS 2
#else
#define DELAY_NOPS 0
#endif
#if NU021I_CASE == 7 || NU021I_CASE == 11
#define OCR_A 0x21
#define OCR_B 0x22
#else
#define OCR_A 0x20
#define OCR_B 0x21
#endif
.text
.global start
start:
  rjmp init
.org 0x60
init:
  ldi r16,0
  sts 0x80,r16       /* WGM0 */
  sts 0x89,r16       /* OCR1AH */
  ldi r16,OCR_A
  sts 0x88,r16
  ldi r16,0
  sts 0x8b,r16       /* OCR1BH */
  ldi r16,OCR_B
  sts 0x8a,r16
#if START_RUNNING
  ldi r16,TIMER_CS
#else
  ldi r16,0
#endif
  sts 0x81,r16       /* TCCR1B: stopped or selected divider */
  ldi r16,0
  sts 0x85,r16       /* TCNT1H -> TEMP */
  ldi r17,0x26
  out 0x16,r17       /* W1C any initial flags */
  ldi r16,0x20
  ldi r24,0xa5
#if START_RUNNING
  .rept DELAY_NOPS
  nop
  .endr
#endif
  sts 0x84,r16       /* commit TCNT1=0x20 with timer stopped/running */
#if !START_RUNNING
  .rept DELAY_NOPS
  nop
  .endr
  ldi r16,TIMER_CS
  sts 0x81,r16       /* enable timer from stop, no implicit prescaler reset */
#endif
  ldi r25,0x5a
  in r26,0x16       /* first early flags */
  nop
  in r27,0x16
  .rept 16
  nop
  .endr
  in r28,0x16
  .rept 90
  nop
  .endr
  in r29,0x16       /* long after all /8 comparator deadlines */
  lds r20,0x84      /* guest read TCNT1L (latches shared high byte) */
  lds r21,0x85
  ldi r18,0x5a
  sts 0x101,r18
done:
  rjmp done
