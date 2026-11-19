/* Shared live CTC guest: same exact word list is constructed in WeightOS.
 * READS are real guest LDS instructions, NOT raw simulator memory peeks.
 * WGM selects mode 4 (OCR1A TOP) or 12 (ICR1 TOP), CS=1 or 2.
 */
#ifndef WGM
#error WGM must be defined as 4 or 12
#endif
#ifndef CLOCK_SELECT
#error CLOCK_SELECT must be defined as 1 or 2
#endif
#if WGM == 4
#define CTC_CONTROL 0x08
#define TOP_HIGH 0x0089
#define TOP_LOW 0x0088
#elif WGM == 12
#define CTC_CONTROL 0x18
#define TOP_HIGH 0x0087
#define TOP_LOW 0x0086
#else
#error unsupported WGM
#endif
.text
.global start
start:
    ldi r16, CTC_CONTROL
    sts 0x0081, r16
    ldi r16, 0x00
    sts TOP_HIGH, r16
    ldi r16, 0x03
    sts TOP_LOW, r16
    ldi r16, (CTC_CONTROL | CLOCK_SELECT)
    sts 0x0081, r16
    nop
    lds r17, 0x0084       /* CPU-visible TCNT1 low */
    lds r18, 0x0036       /* CPU-visible TIFR1 */
    nop
    lds r19, 0x0084
    lds r20, 0x0036
    nop
    lds r21, 0x0084
    lds r22, 0x0036
    nop
    lds r23, 0x0084
    lds r24, 0x0036
    break
