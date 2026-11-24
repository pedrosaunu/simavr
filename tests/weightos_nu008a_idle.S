/* Idle sleep, three internally generated timer wakeups, no host stimulus.
 * avr sim sleep latency is observed separately from datasheet timing. */
.text
.global start
start:
 rjmp init
.org 0x2c
 rjmp isr_a
.org 0x30
 rjmp isr_b
.org 0x34
 rjmp isr_ovf
.org 0x70
init:
 cli
 ldi r16,1
 out 0x3e,r16
 ldi r16,0x3f
 out 0x3d,r16
 ldi r16,0x69
 sts 0x101,r16
 ldi r16,0xd2
 sts 0x12b,r16
 ldi r20,0
 ldi r21,0
 ldi r22,0
 ldi r23,0
 ldi r24,0
 ldi r25,0
 ldi r28,0
round:
 cli
 ldi r16,0
 sts 0x6f,r16
 sts 0x81,r16
 sts 0x80,r16
 ldi r16,0x27
 out 0x16,r16
 ldi r16,0xff
 sts 0x89,r16
 sts 0x8b,r16
 sts 0x85,r16
 ldi r16,0xa0
 sts 0x84,r16
 ldi r16,0xc0
 sts 0x88,r16
 ldi r16,0xd0
 sts 0x8a,r16
 inc r28
 sts 0x100,r28
 cpi r28,1
 breq mask_a
 cpi r28,2
 breq mask_b
 ldi r16,1
 rjmp mask_set
mask_a:
 ldi r16,2
 rjmp mask_set
mask_b:
 ldi r16,4
mask_set:
 sts 0x6f,r16
 ldi r16,1
 out 0x33,r16
 sts 0x81,r16
 ldi r16,0x5a
 ldi r17,0xa5
 sei
 sleep
 inc r25
 cli
 ldi r16,0
 out 0x33,r16
 sts 0x81,r16
 cpi r28,3
 breq done
 rjmp round
done:
 nop
 rjmp done
.org 0x180
isr_a:
 inc r20
 rjmp common
isr_b:
 inc r21
 rjmp common
isr_ovf:
 inc r22
common:
 in r23,0x16
 inc r24
 reti
