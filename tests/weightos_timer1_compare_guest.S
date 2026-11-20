/* Guest-driven Timer1 COMPA/COMPB interrupt priority, no vector injection.
 * Compiled from reset in 3 forms (TIMSK1 0x02/0x04/0x06).
 * The initial OCR1A/B=0 matches of the normal-mode timer set both flags;
 * CPU interrupt arbitration chooses COMPA ahead of COMPB when enabled.
 */
#ifndef IRQ_MASK
#define IRQ_MASK 6
#endif
.text
.global start
start:
    rjmp init
.org 0x2c
compa_isr:
    inc r21
    reti
.org 0x30
compb_isr:
    inc r22
    reti
.org 0x34
ovf_isr:
    inc r23
    reti
.org 0x40
init:
    ldi r16,IRQ_MASK
    sts 0x006f,r16       /* TIMSK1 enable selected sources */
    ldi r16,0x01
    sts 0x0081,r16       /* TCCR1B CS1 undivided, WGM0 */
    sei
loop:
    nop
    rjmp loop
