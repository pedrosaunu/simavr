/* Internal Timer1-to-GPIO output ownership seam.
 * State remains owned by avr_timer_t. No clock or scheduler policy here. */
#ifndef AVR_TIMER_OUTPUT_H
#define AVR_TIMER_OUTPUT_H
#include "avr_timer.h"
void avr_timer_output_sync(avr_timer_t *p);
void avr_timer_output_emit(avr_timer_t *p,unsigned ch,avr_irq_t *irq,uint32_t value);
#endif
