/* Model-independent AVR opcode boundary exercise. Data addr 0x3e is
 * I/O addr 0x1e (GPIOR0 on the tested classic ATmega families).
 */
.text
.global start
start:
    ldi r16, 0x5a
    out 0x1e, r16        ; single-clock mapped I/O store
    sts 0x003e, r16      ; two-clock mapped I/O store
    ldi r26, 0x3e
    ldi r27, 0
    st X, r16            ; two-clock indirect X
    ldi r28, 0x3e
    ldi r29, 0
    st Y, r16            ; two-clock indirect Y
    ldi r30, 0x3e
    ldi r31, 0
    st Z, r16            ; two-clock indirect Z
    cbi 0x1e, 0         ; two-clock RMW
    sbi 0x1e, 0         ; two-clock RMW
    ldi r28, 0x3b
    std Y+3, r16         ; two-clock STD Y+q
    ldi r30, 0x3b
    std Z+3, r16         ; two-clock STD Z+q
loop:
    rjmp loop
