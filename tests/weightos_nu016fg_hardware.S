/* Hardware-only observation suffix. Original stimulus prefix unchanged.
 * ATmega328P, 16 MHz, USART0 9600 8N1. Reset before each capture.
 * Output: A5 5A PHASE TCNTL TCNTH PORTB PINB. All guest samples
 * were taken before enabling USART; only PB1/PB2 measured during prefix.
 */
#include "weightos_nu016f_boundaries.S"
 ldi r16,0
 sts 0xc5,r16
 sts 0xc0,r16
 ldi r16,103
 sts 0xc4,r16
 ldi r16,6
 sts 0xc2,r16
 ldi r16,8
 sts 0xc1,r16
 .macro tx source
1:
 lds r16,0xc0
 sbrs r16,5
 rjmp 1b
 sts 0xc6,\source
 .endm
 ldi r17,0xa5
 tx r17
 ldi r17,0x5a
 tx r17
 ldi r17,PHASE
 tx r17
 tx r21
 tx r22
 tx r23
 tx r20
2:
 rjmp 2b
