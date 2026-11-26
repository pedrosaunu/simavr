.text
.global start
start:
 cli
 ldi r16, (WGM & 3)
 sts 0x80,r16
 ldi r16, ((WGM & 12) << 1)
 sts 0x81,r16
 ldi r16,0
 sts 0x87,r16
 ldi r16,7
 sts 0x86,r16
 ldi r16,0
 sts 0x89,r16
#if WGM == 9 || WGM == 11
 ldi r16,7
#else
 ldi r16,3
#endif
 sts 0x88,r16
 ldi r16,0
 sts 0x8b,r16
 ldi r16,2
 sts 0x8a,r16
 ldi r16,39
 out 0x16,r16
 ldi r16, ((WGM & 12) << 1) | CS
 sts 0x81,r16
 ldi r24,0
 ldi r25,90
sample:
 nop
 nop
 nop
 nop
 lds r18,0x84
 lds r19,0x85
 in r20,0x16
 inc r24
 rjmp sample
