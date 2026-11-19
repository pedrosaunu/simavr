/* One 2-clock write retires at cycle 3, after the LDI at cycle 1.
 * Host schedules independent peripheral events at cycles 2 and 4.
 * The cycle-2 event MUST observe pre-store state; the cycle-4 event
 * MUST observe post-store state. No model-derived expected values.
 */
.text
.global start
start:
    ldi r16, 0xa5
    sts 0x003e, r16
    nop
loop:
    nop
    rjmp loop
