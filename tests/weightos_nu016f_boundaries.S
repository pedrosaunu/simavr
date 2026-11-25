/* WGM14 /1: phase-selectable OCR update, stop/resume, COM disconnect.
 * PORTB sentinel PB1 high before connecting, PB2 high written while stopped.
 * r24 stage; r20=PINB, r23=PORTB, r21/r22=TCNT. */
.text
.global start
start:
 cli
 ldi r16,6
 out 4,r16
 ldi r16,2
 out 5,r16
 ldi r16,24
 sts 0x81,r16
 ldi r16,0
 sts 0x87,r16
 ldi r16,7
 sts 0x86,r16
 ldi r16,0
 sts 0x89,r16
 ldi r16,2
 sts 0x88,r16
 ldi r16,0
 sts 0x8b,r16
 ldi r16,5
 sts 0x8a,r16
 ldi r16,0xa2
 sts 0x80,r16
 ldi r24,1
 in r23,5
 in r20,3
 ldi r16,25
 sts 0x81,r16
 .rept 24
 nop
 .endr
 ldi r24,2
 ldi r16,1
 .rept PHASE
 nop
 .endr
 sts 0x8a,r16
 .rept 24
 nop
 .endr
 ldi r24,3
 ldi r16,24
 sts 0x81,r16
 in r23,5
 in r20,3
 lds r21,0x84
 lds r22,0x85
 ldi r16,4
 out 5,r16
 in r23,5
 in r20,3
 .rept 4
 nop
 .endr
 ldi r24,4
 ldi r16,25
 sts 0x81,r16
 .rept 12
 nop
 .endr
 ldi r16,24
 sts 0x81,r16
 ldi r24,5
 ldi r16,2
 sts 0x80,r16
 in r23,5
 in r20,3
 ldi r25,90
 nop
