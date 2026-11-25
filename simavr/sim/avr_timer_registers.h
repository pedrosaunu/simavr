/* Internal Timer1 register-access seam. TEMP is owned by avr_timer_t. */
#ifndef AVR_TIMER_REGISTERS_H
#define AVR_TIMER_REGISTERS_H
#include "avr_timer.h"
int avr_timer_temp_profile(avr_timer_t *p);
void avr_timer_temp_commit_high(avr_timer_t *p, avr_io_addr_t high);
void avr_timer_temp_install(avr_timer_t *p);
#endif
