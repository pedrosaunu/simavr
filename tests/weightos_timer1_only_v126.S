.text
.global start
start:
    rjmp init
.org 0x34
    rjmp timer1_isr
.org 0x40
    rjmp timer0_isr
.org 0x100
init:
    ldi r16, 0xff
    out 0x04, r16      ; DDRB full output
    ldi r16, 1
    sts 0x006f, r16    ; TIMSK1 overflow
    nop ; other overflow interrupt disabled
    out 0x25, r16     ; Timer0 /1
    sts 0x0081, r16   ; Timer1 /1
    ldi r16, 0xff
    sts 0x0085, r16    ; TCNT1H
    ldi r16, 0x00
    sts 0x0084, r16    ; TCNT1L
    out 0x26, r16     ; TCNT0=0
    sei
loop:
    nop
    rjmp loop

timer1_isr:
    inc r21
    out 0x05, r21
    ldi r16,0xff
    sts 0x0085,r16
    ldi r16,0x00
    sts 0x0084,r16
    reti

timer0_isr:
    inc r22
    out 0x05,r22
    reti
