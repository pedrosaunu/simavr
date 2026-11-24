/* NU-008B: exact low/high SRAM boundary accesses; no invalid hardware
 * access in this independent witness. Invalid stack operations are separate
 * fail-closed neural controls, not claimed as ATmega hardware semantics. */
#ifndef STACK_TOP
#define STACK_TOP 0x13f
#endif
.text
.global start
start:
 rjmp init
.org 0x70
init:
 cli
 ldi r16,1
 out 0x3e,r16
 ldi r16,1
 out 0x3d,r16
 ldi r16,0x5a
 ldi r17,0xc3
 ldi r20,0
 ldi r21,0
 ldi r22,0
 ldi r23,0
 ldi r18,0xd2
 sts 0x102,r18
 ldi r18,0x69
 sts STACK_TOP-2,r18
 ldi r18,1
 sts 0x103,r18
 call low_leaf
 ldi r18,0
 out 0x3d,r18
 push r16
 pop r21
 ldi r18,hi8(STACK_TOP)
 out 0x3e,r18
 ldi r18,lo8(STACK_TOP)
 out 0x3d,r18
 rcall high_leaf
 ldi r30,lo8(pm(high_leaf))
 ldi r31,hi8(pm(high_leaf))
 icall
 ldi r23,0xa5
done:
 nop
 rjmp done
low_leaf:
 inc r20
 ret
high_leaf:
 inc r22
 ret
