/* Runtime mode changes with guest-owned register writes.
 * Start normal, stop and configure CTC4, switch running to normal,
 * then stopped configure CTC12 using ICR1 as TOP. */
.text
.global start
start:
 cli
 ldi r16,0
 sts 0x80,r16
 sts 0x81,r16
 sts 0x89,r16
 ldi r16,3
 sts 0x88,r16
 ldi r16,0
 sts 0x8b,r16
 ldi r16,1
 sts 0x8a,r16
 sts 0x81,r16
 .rept 6
 nop
 .endr
 ldi r16,0
 sts 0x81,r16
 sts 0x85,r16
 sts 0x84,r16
 ldi r16,8
 sts 0x81,r16
 ldi r16,9
 sts 0x81,r16
 .rept 12
 nop
 .endr
 ldi r16,1
 sts 0x81,r16
 .rept 6
 nop
 .endr
 ldi r16,24
 sts 0x81,r16
 ldi r16,0
 sts 0x87,r16
 ldi r16,5
 sts 0x86,r16
 ldi r16,0
 sts 0x85,r16
 sts 0x84,r16
 ldi r16,25
 sts 0x81,r16
 .rept 12
 nop
 .endr
 ldi r16,0
 sts 0x81,r16
 ldi r25,0x5a
 nop
