/* WGM8 TOP=7, clock-select lifecycle. Host drives settled T1 samples.
 * Stages: rising -> falling -> stopped -> rising.
 * No register changes except TCCR1B clock selection after setup. */
.text
.global start
start:
 cli
 ldi r16,0
 sts 0x80,r16
 ldi r16,16
 sts 0x81,r16
 ldi r16,0
 sts 0x87,r16
 ldi r16,7
 sts 0x86,r16
 ldi r16,0
 sts 0x89,r16
 ldi r16,3
 sts 0x88,r16
 ldi r16,0
 sts 0x8b,r16
 ldi r16,2
 sts 0x8a,r16
 ldi r16,39
 out 0x16,r16
 ldi r24,0
stage0:
 ldi r23,0
 ldi r16,23
 sts 0x81,r16
 ldi r17,16
 ldi r25,90
sample0:
 nop
 nop
 nop
 nop
 lds r18,0x84
 lds r19,0x85
 in r20,0x16
 inc r24
 dec r17
 brne sample0
stage1:
 ldi r23,1
 ldi r16,22
 sts 0x81,r16
 ldi r17,16
sample1:
 nop
 nop
 nop
 nop
 lds r18,0x84
 lds r19,0x85
 in r20,0x16
 inc r24
 dec r17
 brne sample1
stage2:
 ldi r23,2
 ldi r16,16
 sts 0x81,r16
 ldi r17,16
sample2:
 nop
 nop
 nop
 nop
 lds r18,0x84
 lds r19,0x85
 in r20,0x16
 inc r24
 dec r17
 brne sample2
stage3:
 ldi r23,3
 ldi r16,23
 sts 0x81,r16
 ldi r17,16
sample3:
 nop
 nop
 nop
 nop
 lds r18,0x84
 lds r19,0x85
 in r20,0x16
 inc r24
 dec r17
 brne sample3
done:
 rjmp done
