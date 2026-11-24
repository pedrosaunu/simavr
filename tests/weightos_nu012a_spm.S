/* Real addresses: 512-byte boot section at byte 0x7e00.
 * Two application pages at 0x0000..0x00ff, one mapped boot page.
 * SPM ready/busy delays are not modeled by simavr. */
.text
.global start
start:
 jmp boot
.org 0x80
 .word 0x0000,0x0000,0x1234
.org 0x7e00
boot:
 cli
 ldi r30,0x80
 ldi r31,0
 ldi r16,3
 out 0x37,r16
 spm
 ldi r16,0x8a
 mov r0,r16
 ldi r16,0xe5
 mov r1,r16
 ldi r16,1
 out 0x37,r16
 spm
 adiw r30,2
 ldi r16,0xff
 mov r0,r16
 ldi r16,0xcf
 mov r1,r16
 ldi r16,1
 out 0x37,r16
 spm
 ldi r16,5
 out 0x37,r16
 spm
 ldi r16,0x11
 out 0x37,r16
 spm
 ldi r30,0x80
 lpm r20,Z+
 lpm r21,Z
 ldi r30,0x40
 ldi r31,0
 ijmp
