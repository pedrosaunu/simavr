/* NU-021H: genuine AVR firmware tests TCNT1 post-write compare suppression.
 * Nine independent binaries. CPU starts from reset, no host IRQ/flag seeding.
 * Cases 6/8 use 2-cycle RJMP; case 7 uses 2-cycle LDS after a guest snapshot.
 */
#ifndef NU021H_CASE
#define NU021H_CASE 0
#endif
#if NU021H_CASE == 0 || NU021H_CASE == 6
# define OCRA 0x20
# define OCRB 0x21
#elif NU021H_CASE == 1 || NU021H_CASE == 7
# define OCRA 0x21
# define OCRB 0x22
#elif NU021H_CASE == 2
# define OCRA 0x20
# define OCRB 0x20
#elif NU021H_CASE == 3
# define OCRA 0x22
# define OCRB 0x23
#elif NU021H_CASE == 4 || NU021H_CASE == 8
# define OCRA 0x20
# define OCRB 0x22
#elif NU021H_CASE == 5
# define OCRA 0x21
# define OCRB 0x20
#else
# error Invalid case
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
  ldi r16,OCRA
  sts 0x88,r16       /* OCR1A */
  ldi r16,0
  sts 0x8b,r16       /* OCR1BH */
  ldi r16,OCRB
  sts 0x8a,r16       /* OCR1B */
  ldi r16,1
  sts 0x81,r16       /* TCCR1B /1 */
  ldi r16,0
  sts 0x85,r16       /* TCNT1H staged */
  ldi r17,0x26
  out 0x16,r17       /* clear initial flags */
  ldi r16,0x20
  ldi r24,0xa5
  sts 0x84,r16       /* commit TCNT1 = 0x20 */
  ldi r25,0x5a
  in r26,0x16
#if NU021H_CASE == 6 || NU021H_CASE == 8
  rjmp second_read
#elif NU021H_CASE == 7
  lds r19,0x100
#else
  nop
#endif
second_read:
  in r27,0x16
  nop
  in r28,0x16
  .rept 9
    nop
  .endr
  in r29,0x16
  lds r20,0x84
  lds r21,0x85
  ldi r18,0x5a
  sts 0x101,r18
done:
  rjmp done
