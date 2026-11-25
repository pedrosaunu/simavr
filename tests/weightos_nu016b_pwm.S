/* Guest-controlled stopped setup, then WGM14 /1, TOP=3.
 * Sample actual pin and timer registers after a sustained train.
 * Runtime OCR1B update; exit via stopped CTC12 then normal. */
.text
.global start
start:
 cli
 ldi r16,6
 out 4,r16
 ldi r16,24
 sts 0x81,r16
 ldi r16,0
 sts 0x87,r16
 ldi r16,3
 sts 0x86,r16
 ldi r16,0
 sts 0x89,r16
 ldi r16,1
 sts 0x88,r16
 ldi r16,0
 sts 0x8b,r16
 ldi r16,INITIAL_B
 sts 0x8a,r16
 ldi r16,0xa2
 sts 0x80,r16
 ldi r16,25
 sts 0x81,r16
 .rept 20
 nop
 .endr
 in r20,3
 lds r21,0x84
 lds r22,0x85
 ldi r16,FINAL_B
 sts 0x8a,r16
 .rept 20
 nop
 .endr
 in r20,3
 lds r21,0x84
 lds r22,0x85
 ldi r16,24
 sts 0x81,r16
 ldi r16,0
 sts 0x80,r16
 sts 0x81,r16
 ldi r25,0x5a
 nop
