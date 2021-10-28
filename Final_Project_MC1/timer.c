/*
 * timer.c
 *
 *  Created on: Oct 21, 2021
 *      Author: TOSHIBA PC
 */

#include "timer.h"
#include <avr/io.h>

/*******************************************************************************
 *                       Global variables                           *
 *******************************************************************************/
volatile uint32 SECONDS_T1=0;
volatile uint32 SECONDS_T0_MC1=0;
volatile uint32 SECONDS_T0_MC2=0;
static volatile void (*g_callBackPtr)(void)=NULL_PTR;

/*******************************************************************************
 *                       ISR Definition                          *
 *******************************************************************************/
/*ISR (TIMER0_OVF_vect)   { 	   SECONDS_T0_MC1 ++; SECONDS_T0_MC2 ++;    }

ISR (TIMER0_COMP_vect)  {		SECONDS_T0_MC1 ++; SECONDS_T0_MC2 ++;	  }
*/
ISR(TIMER0_OVF_vect)
{
	//TCNT0 = g_initial_value ;
	if(g_callBackPtr != NULL_PTR)
	{
		(*g_callBackPtr)();
	}
}

ISR(TIMER0_COMP_vect)
{
	if(g_callBackPtr != NULL_PTR)
	{
		(*g_callBackPtr)();
	}
}

/*******************************************************************************
 *                       Function Definition                           *
 *******************************************************************************/
void Timer0_init(const TimerConfig  *configuration){
	TCCR0 = ((TCCR0 & ~0x07) | configuration->clock);  //ADJUSTS PRESCALAR BITS
	TCCR0 = ((TCCR0 & ~0xC8) | configuration->mode);  //ADJUSTS MODE (FOC0,WGM00,WGM01 bits)
	TCNT0 = configuration->initial_value; // initial value for counting

	if (configuration->mode == NORMAL)
		{
			TIMSK|=(1<<TOIE0);							//TIMER OVERFLOW INTERRUPT ENABLE
		}

		if (configuration->mode == CTC)
		{
			TIMSK|=(1<<OCIE0);
			OCR0= configuration->compare_value;        //SET COMPARE VALUE
		}
}
void Timer0_deinit(){
	TCCR0=0; TCNT0=0; TIMSK &=~(1<<TOIE0) & ~(1<<OCIE0);

}
void Timer0_stop(){
	TCCR0 = (TCCR0 & ~0x07);
}






void Timer1_init(const TimerConfig *configuration)
{
	TCCR1B = (TCCR1B & ~0x07) | configuration->clock; //ADJUSTS PRESACALAR BITS

	TCCR1B = (TCCR1B & ~0x08) | (configuration->mode); //FOR WGM12 IF CTC MODE 1 / NORMAL MODE 0

	TCCR1A = (1<<FOC1A);  							  //TIMER1 ONLY SUPPORTS CTC MODE AND NORMAL MODE

    TCNT1 = configuration->initial_value;

	if (configuration->mode == NORMAL)
	{
		TIMSK|=(1<<TOIE1);							//TIMER OVERFLOW INTERRUPT ENABLE
	}

	if (configuration->mode == CTC)
	{
		OCR1A= configuration->compare_value;        //SET COMPARE VALUE
		TIMSK|=(1<<OCIE1A);
	}
}

void Timer1_deinit()
{
	TCCR1B=0; TCCR1A=0; TCNT0=0; TIMSK &=~(1<<TOIE1) & ~(1<<OCIE1A);
}



void Timer1_stop()
{
	TCCR1B = (TCCR1B & ~0x07);
}


void Timer0_setCallBack(void(*a_ptr)(void))
{
	g_callBackPtr=a_ptr;
}
