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
#include "avr_timer_output.h"
#include "avr_ioport.h"

/* Only ATmega328P Timer1 WGM14 COM=2/3, PB1/2. Other profiles retain
 * their existing IRQ wiring. COM changes are independent of CS. */
void
avr_timer_output_sync(avr_timer_t *p)
{
    avr_t *avr=p->io.avr;
    if (p->name!='1' || (strcmp(avr->mmcu,"atmega328p")!=0 && strcmp(avr->mmcu,"atmega328")!=0))
        return;
    uint8_t mode=avr_regbit_get_array(avr,p->wgm,ARRAY_SIZE(p->wgm));
    for (unsigned ch=0;ch<2;ch++) {
        avr_regbit_t bit=p->comp[ch].com_pin;
        uint8_t mask=1u<<ch;
        uint8_t com=avr_regbit_get(avr,p->comp[ch].com);
        int enabled=mode==14 && (com==2 || com==3);
        if (enabled==!!(p->nu016h_override&mask)) continue;
        avr_ioport_getirq_t req={.bit=bit};
        if (avr_ioctl(avr,AVR_IOCTL_IOPORT_GETIRQ_REGBIT,&req)<=0) continue;
        avr_irq_t *src=p->io.irq+TIMER_IRQ_OUT_COMP+ch;
        avr_ioport_override_t change={.bit=bit,.enabled=enabled,
            .value=!!(p->nu016h_level&mask)};
        if (avr_ioctl(avr,AVR_IOCTL_IOPORT_SET_OVERRIDE,&change)<0) continue;
        if (enabled) {
            avr_unconnect_irq(src,req.irq[0]);
            p->nu016h_override|=mask;
        } else {
            avr_connect_irq(src,req.irq[0]);
            p->nu016h_override&=~mask;
        }
    }
}

void
avr_timer_output_emit(avr_timer_t *p,unsigned ch,avr_irq_t *irq,uint32_t value)
{
    uint8_t mask=1u<<ch;
    if (p->nu016h_override&mask) {
        if (value&1) p->nu016h_level|=mask;
        else p->nu016h_level&=~mask;
        avr_ioport_override_t change={.bit=p->comp[ch].com_pin,.enabled=1,
            .value=!!(value&1)};
        avr_ioctl(p->io.avr,AVR_IOCTL_IOPORT_SET_OVERRIDE,&change);
    }
    avr_raise_irq(irq,value);
}

