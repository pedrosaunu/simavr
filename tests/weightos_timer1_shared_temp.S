/* ATmega328P Timer1 shared high-byte TEMP, CPU-visible LDS/STS only.
 * Clock CS=0 throughout, mode 14 uses ICR1 as TOP, no interrupts.
 * Expected GPR values independent of simavr storage representation.
 */
.text
.global start
start:
    ldi r16, 0x18
    sts 0x0081, r16       ; WGM13/WGM12
    ldi r16, 0x02
    sts 0x0080, r16       ; WGM11: WGM14, ICR1 TOP
    ldi r16, 0x12
    sts 0x0089, r16       ; OCR1AH stages TEMP, not OCR1A
    lds r17, 0x0089       ; OCR1AH direct read -> 0x00; TEMP stays 0x12
    ldi r16, 0x34
    sts 0x008a, r16       ; OCR1BL commits 0x1234 across register names
    lds r18, 0x008a       ; 0x34
    lds r19, 0x008b       ; OCR1BH direct -> 0x12
    ldi r16, 0x56
    sts 0x0085, r16       ; TCNT1H stages TEMP=0x56
    ldi r16, 0x78
    sts 0x0086, r16       ; ICR1L commits ICR1=0x5678
    lds r20, 0x0086       ; low -> 0x78, TEMP<-ICR1H=0x56
    lds r21, 0x0087       ; ICR1H reads TEMP -> 0x56
    ldi r16, 0xab
    sts 0x0089, r16       ; TEMP=0xab
    ldi r16, 0xcd
    sts 0x0088, r16       ; OCR1A=0xabcd
    ldi r16, 0xee
    sts 0x008b, r16       ; TEMP=0xee, OCR1B remains 0x1234
    lds r22, 0x0089       ; OCR1AH direct read -> 0xab; TEMP stays ee
    lds r23, 0x0087       ; ICR1H sees TEMP -> 0xee
    lds r24, 0x0086       ; low ICR1 -> 0x78, TEMP=0x56 again
    lds r25, 0x0085       ; TCNT1H sees shared TEMP -> 0x56
    ldi r26, 0xa5
    out 0x1e, r26        ; GPIOR0 completion marker
loop:
    nop
    rjmp loop
