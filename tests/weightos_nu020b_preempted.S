/* NU-020B: real IRQ preempts five Timer1 flag writes. Each write
 * must take effect only on its eventual retirement, never on IRQ entry.
 * Rounds 2/4 stop Timer1; others run /1. No host injection except PB0. */
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
 ldi r16,0
 sts 0x80,r16
 sts 0x89,r16
 sts 0x8b,r16
 ldi r16,0x18
 sts 0x88,r16
 ldi r16,0x28
 sts 0x8a,r16
 ldi r26,0x36
 ldi r27,0
 ldi r24,0
 ldi r28,0
 ldi r29,0
round:
 cli
 ldi r16,0
 sts 0x6f,r16
 sts 0x81,r16
 ldi r16,0x27
 out 0x16,r16
 ldi r16,0xff
 sts 0x85,r16
 ldi r16,0xd0
 sts 0x84,r16
 ldi r16,0x41
 sts 0x81,r16
 inc r28
 sts 0x100,r28
 .rept 110
 nop
 .endr
 sbrs r28,0
 rjmp stop_timer
 rjmp enable
stop_timer:
 ldi r16,0x40
 sts 0x81,r16
enable:
 ldi r16,0x20
 sts 0x6f,r16
 ldi r16,0x27
 cpi r28,1
 breq out_case
 cpi r28,2
 breq sts_case
 cpi r28,3
 breq sbi_case
 cpi r28,4
 breq cbi_case
 sei
 nop
 st X,r16
 rjmp observe
out_case:
 sei
 nop
 out 0x16,r16
 rjmp observe
sts_case:
 sei
 nop
 sts 0x36,r16
 rjmp observe
sbi_case:
 sei
 nop
 sbi 0x16,1
 rjmp observe
cbi_case:
 sei
 nop
 cbi 0x16,1
observe:
 in r21,0x16
 lds r22,0x6f
 mov r29,r28
 cpi r28,5
 breq done
 rjmp round
done:
 nop
 rjmp done
.org 0x280
capture:
 push r16
 in r16,0x3f
 push r16
 in r20,0x16
 lds r23,0x6f
 inc r24
 pop r16
 out 0x3f,r16
 pop r16
 reti
