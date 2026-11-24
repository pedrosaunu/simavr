/* NU-021O: 3 rounds of concurrent CAPT/A/B/OVF flags.
 * Two capture edges each round, the second inside the outer ISR.
 * Round 2 uses SEI/CLI negative control rather than nesting.
 * Firmware alone sets masks and counter, and saves/restores context. */
.text
.global start
start:
 rjmp init
.org 0x28
 rjmp capture
.org 0x2c
 rjmp compa
.org 0x30
 rjmp compb
.org 0x34
 rjmp overflow
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
 ldi r20,0
 ldi r21,0
 ldi r22,0
 ldi r23,0
 ldi r24,0
 ldi r25,0
 ldi r26,0
 ldi r27,0
 ldi r28,0
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
 ldi r29,0
 ldi r16,0x41
 sts 0x81,r16
 inc r28
 sts 0x100,r28
 .rept 120
 nop
 .endr
 ldi r16,0x20
 sts 0x6f,r16
 sei
wait:
 nop
 cp r24,r28
 brne wait
 cpi r28,3
 breq done
 rjmp round
done:
 nop
 rjmp done
.org 0x200
capture:
 push r16
 in r16,0x3f
 push r16
 push r17
 inc r20
 inc r29
 cpi r29,1
 brne capture_exit
 ldi r17,2
 sts 0x6f,r17
 call helper
 cli
 ldi r17,0x21
 cpi r28,2
 brne enable_end
 ldi r17,0x27
enable_end:
 sts 0x6f,r17
capture_exit:
 inc r25
 pop r17
 pop r16
 out 0x3f,r16
 pop r16
 reti
helper:
 push r18
 ldi r18,0x99
 cpi r28,2
 breq suppressed
 sei
 nop
 rjmp hold
suppressed:
 sei
 cli
hold:
 .rept 40
 nop
 .endr
 cli
 pop r18
 ret
compa:
 push r16
 in r16,0x3f
 push r16
 inc r21
 cpi r28,2
 breq compa_exit
 ldi r16,4
 sts 0x6f,r16
 sei
 nop
 cli
compa_exit:
 pop r16
 out 0x3f,r16
 pop r16
 reti
compb:
 push r16
 in r16,0x3f
 push r16
 inc r22
 pop r16
 out 0x3f,r16
 pop r16
 reti
overflow:
 push r16
 in r16,0x3f
 push r16
 inc r23
 inc r24
 pop r16
 out 0x3f,r16
 pop r16
 reti
