/* NU-020: WGM0/CS1 Timer1 W1C from genuine guest AVR instructions.
 * Modes OUT/SBI/CBI/STS/ST X with IN/LDS/LD X readback.
 * RUNNING_TIMER=1 retains CS1 while W1C executes.
 */
#ifndef CLEAR_STYLE
#define CLEAR_STYLE 0
#endif
#ifndef RUNNING_TIMER
#define RUNNING_TIMER 0
#endif
.text
.global start
start:
 rjmp init
.org 0x40
init:
 ldi r16,1
 sts 0x0081,r16
 nop
 nop
#if RUNNING_TIMER
 nop
 nop
 nop
#else
 ldi r16,0
 sts 0x0081,r16
#endif
#if CLEAR_STYLE==4 || CLEAR_STYLE==6
 ldi r26,0x36
 ldi r27,0x00
#endif
 in r20,0x16
#if CLEAR_STYLE==0
 ldi r17,0x02
 out 0x16,r17
#elif CLEAR_STYLE==1
 sbi 0x16,1
#elif CLEAR_STYLE==2
 cbi 0x16,1
#elif CLEAR_STYLE==3 || CLEAR_STYLE==5
 ldi r17,0x02
 sts 0x0036,r17
#elif CLEAR_STYLE==4 || CLEAR_STYLE==6
 ldi r17,0x02
 st X,r17
#else
#error Unsupported_CLEAR_STYLE
#endif
#if CLEAR_STYLE==5
 lds r21,0x0036
#elif CLEAR_STYLE==6
 ld r21,X
#else
 in r21,0x16
#endif
 ldi r17,0
 out 0x16,r17
 in r22,0x16
 ldi r17,0x04
 out 0x16,r17
 in r23,0x16
 lds r24,0x0084       /* guest TCNT1L read triggers timer hardware register */
 lds r25,0x0085       /* guest TCNT1H read sees TEMP high latch */
 ldi r19,0x5a
loop:
 nop
 rjmp loop
