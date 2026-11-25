/*
	avr_timer.c

	Handles the 8 bits and 16 bits AVR timer.
	Handles
	+ CDC
	+ Fast PWM

	Copyright 2008-2012 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "avr_timer_registers.h"

int avr_timer_temp_profile(avr_timer_t *p)
{
    return p->name=='1' && p->r_tcnth &&
        (!strcmp(p->io.avr->mmcu,"atmega328") || !strcmp(p->io.avr->mmcu,"atmega328p"));
}

/* CPU high writes stage TEMP, not the architectural register.
 * Existing TCNT/ICR low-read callbacks snapshot the same byte;
 * OCR reads remain ordinary direct reads. */
static void temp_write(avr_t *avr, avr_io_addr_t addr, uint8_t v, void *param)
{
    (void)avr;(void)addr;
    ((avr_timer_t*)param)->read_temp_high=v;
}

void avr_timer_temp_commit_high(avr_timer_t *p, avr_io_addr_t high)
{
    if (high) p->io.avr->data[high]=p->read_temp_high;
}

static void icr_low_write(avr_t *avr, avr_io_addr_t addr, uint8_t v, void *param)
{
    avr_timer_t *p=param;
    uint8_t wgm=avr_regbit_get_array(avr,p->wgm,ARRAY_SIZE(p->wgm));
    if (wgm!=8 && wgm!=10 && wgm!=12 && wgm!=14) return;
    avr_timer_temp_commit_high(p,p->r_icrh);
    avr_core_watch_write(avr,addr,v);
}

void avr_timer_temp_install(avr_timer_t *p)
{
    if (!avr_timer_temp_profile(p)) return;
    avr_t *avr=p->io.avr;
    avr_register_io_write(avr,p->r_tcnth,temp_write,p);
    if (p->r_icrh) avr_register_io_write(avr,p->r_icrh,temp_write,p);
    if (p->r_icr) avr_register_io_write(avr,p->r_icr,icr_low_write,p);
    for (unsigned ch=0;ch<AVR_TIMER_COMP_COUNT;ch++)
        if (p->comp[ch].r_ocrh)
            avr_register_io_write(avr,p->comp[ch].r_ocrh,temp_write,p);
}
