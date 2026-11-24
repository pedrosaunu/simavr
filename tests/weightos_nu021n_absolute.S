/* NU-021N: 3 rounds of mixed CALL/ICALL with same-vector reentry.
 * Guest sets absolute SP and two SRAM canaries. No host state injection. */
.text
.global start
start:
 rjmp init
.org 0x28
 rjmp capture
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
 ldi r30,lo8(pm(leaf))
 ldi r31,hi8(pm(leaf))
 ldi r16,0x41
 sts 0x81,r16
 ldi r16,0x20
 sts 0x6f,r16
 ldi r16,0x5a
 ldi r17,0xc3
 ldi r18,0x35
 ldi r19,0xa6
 ldi r21,0
 ldi r22,0
 ldi r23,0
 ldi r24,0
 ldi r25,0
 ldi r26,0
 ldi r27,0
 ldi r20,0xa5
 sts 0x100,r20
 ldi r20,0
 sei
main_loop:
 nop
 rjmp main_loop
.org 0x100
capture:
 push r16
 in r16,0x3f
 push r16
 push r17
 inc r20
 inc r21
 ldi r16,0x77
 ldi r17,0x88
 call helper
 cli
 dec r21
 inc r22
 pop r17
 pop r16
 out 0x3f,r16
 pop r16
 reti
helper:
 push r18
 ldi r18,0x99
 inc r23
 icall
 pop r18
 ret
leaf:
 cpi r21,1
 brne inner
 sei
 .rept 48
 nop
 .endr
 cli
inner:
 inc r24
 ret
