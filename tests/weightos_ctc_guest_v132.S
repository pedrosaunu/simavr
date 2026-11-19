/* WGM4/WGM12, CS=1, TOP=3. Guest-visible snapshots, not internal timer mirrors.
 * Read TCNT1L/H with LDS; each LDS consumes two CPU cycles and advances
 * the live counter. This probe is diagnostic, NOT a cycle oracle.
 */
.text
.global start
start:
    ldi r16, 0x00
    sts 0x0081, r16       ; stopped
    sts 0x0080, r16
    sts 0x0085, r16       ; TCNT1H stages 0
    sts 0x0084, r16       ; TCNT1L commits 0
#ifdef WGM12
    ldi r16, 0x18
    sts 0x0081, r16       ; WGM12, ICR1 TOP
    ldi r16, 0
    sts 0x0087, r16
    ldi r16, 3
    sts 0x0086, r16
#else
    ldi r16, 0x08
    sts 0x0081, r16       ; WGM4, OCR1A TOP
    ldi r16, 0
    sts 0x0089, r16
    ldi r16, 3
    sts 0x0088, r16
#endif
    ldi r16, 0x09
#ifdef WGM12
    ldi r16, 0x19
#endif
    sts 0x0081, r16       ; start clock, CS1
    lds r17, 0x0084       ; guest-visible TCNT1L
    lds r18, 0x0085       ; guest-visible TCNT1H/TEMP
    nop
    lds r19, 0x0084
    lds r20, 0x0085
    nop
    lds r21, 0x0084
    lds r22, 0x0085
    ldi r16, 0
    sts 0x0081, r16       ; stop before reporting
    ldi r26, 0xa5
    out 0x1e, r26        ; completion
loop:
    rjmp loop
