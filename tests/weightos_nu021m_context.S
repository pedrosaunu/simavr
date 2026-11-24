/* NU-021M: two nested instances of the same capture ISR, then
 * a third sequential capture. Actual guest saves SREG, data and CALL frames.
 * No host register, interrupt, SRAM or stack writes. */
.text
.global start
start:
 rjmp init
.org 0x28
 rjmp capture
.org 0x70
init:
 cli
 ldi r16,0x41
 sts 0x81,r16
 ldi r16,0x20
 sts 0x6f,r16
 ldi r16,0x5a
 ldi r17,0xc3
 ldi r18,0x35
 ldi r19,0xa6
 ldi r21,0x00
 ldi r22,0x00
 ldi r23,0x00
 ldi r24,0x00
 ldi r25,0x00
 ldi r26,0x00
 ldi r27,0x00
 ldi r20,0xa5
 sts 0x100,r20
 ldi r20,0x00
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
 rcall helper
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
 cpi r21,1
 brne inner
 sei
 .rept 48
 nop
 .endr
 cli
inner:
 inc r24
 pop r18
 ret
