/* Independent guest AVR firmware for end-to-end Timer1 overflow IRQ.
 * Starts at RESET, writes real TIMSK1, TCCR1B, TCNT1H/L, sets I,
 * waits for overflow; ISR increments r21 and returns via RETI.
 * Same compiled little-endian words must run in the neural model.
 */
.text
.global start
start:
    rjmp init
.org 0x34
timer1_ovf:
    inc r21
    reti
.org 0x40
init:
    /* Deliberately isolate overflow: compare registers are programmed
     * before starting the timer, away from the initial/start interval.
     * The earlier default OCR1A/B=0 experiment remains a separate RED.
     */
    ldi r16, 0x80
    sts 0x0089, r16      /* OCR1AH */
    sts 0x008b, r16      /* OCR1BH */
    ldi r17, 0x00
    sts 0x0088, r17      /* OCR1AL, active comparator A = 0x8000 */
    sts 0x008a, r17      /* OCR1BL, active comparator B = 0x8000 */
    ldi r16, 0x01
    sts 0x006f, r16      /* TIMSK1 TOIE1 */
    sts 0x0081, r16      /* TCCR1B CS=1, WGM0 */
    ldi r16, 0xff
    sts 0x0085, r16      /* TCNT1H temporary latch */
    ldi r16, 0xfe
    sts 0x0084, r16      /* TCNT1L commit */
    sei
loop:
    nop
    rjmp loop
