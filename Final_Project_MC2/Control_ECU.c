/*
 * Control_ECU.c
 *
 *  Created on: Oct 23, 2021
 *      Author: TOSHIBA PC
 */



#include "timer.h"
#include "external_eeprom.h"
#include "uart.h"
#include "motor.h"
#include "buzzer.h"

#define DELAY_Keypad 2500
#define DELAY_UART 130

/*******************************************************************************
 *                      Global Variable                                   *
 *******************************************************************************/
volatile uint8 Valid;
/*******************************************************************************
 *                      Function Prototype                                  *
 *******************************************************************************/
void RECEIVE_PW(uint8 PW[] );
void VERIFY_PW( uint8 PW[] , uint8 check_pw[] );
void timer0_isr_fn(void);

volatile int quarter_sec=0;


int main(void){
	uint8 P_W[4];
	uint8 check[4];
	Timer0_setCallBack(timer0_isr_fn);
	buzzer_init();
	EEPROM_init();
	MOTOR_init();
	UART_Config UConfig = {ASYNCH,ENABLED_EVEN,BIT_1};
	UART_init(&UConfig);

	TimerConfig T0_Configuration={NORMAL,F_CPU_1024,0,7813};
	Timer0_init(&T0_Configuration); 			 //Initialize timer0
	SREG |= (1<<7);

	// get pass and check it
	do{
		RECEIVE_PW( P_W );        	// RECIEVE FIRST PW USER SENDS
		RECEIVE_PW( check );        // RECIEVE VERIFYING PW USER SENDS
		VERIFY_PW (P_W,check);		//CHECK IF PW'S SENT FROM THE HMI MATCH
	}while (Valid == 0);


	while(1){
		uint8 command=UART_receiveByte();
		if(command== '-')
		{
			do{
					RECEIVE_PW( P_W );        	// RECIEVE FIRST PW USER SENDS
					RECEIVE_PW( check );        // RECIEVE VERIFYING PW USER SENDS
					VERIFY_PW (P_W,check);		//CHECK IF PW'S SENT FROM THE HMI MATCH
			    }while (Valid == 0);
		}

		// SAVE PASS IN EEPROM
		for(uint8 i=0;i<4;i++)
		{
			EEPROM_writeByte((0X0090+i), P_W[i]);
			_delay_ms(DELAY_UART);
		}
		if(command== '+'){

			/* GET PASSWORD IN EEPROM AND SAVE IT IN A VARIABLE TO CHECK PW USER SENT */
			for (uint8 i = 0; i < 4; i++)
			{
				EEPROM_readByte( (0x0090 + i), (P_W+i));
				_delay_ms(100);
			}
			// the user 3 chance for password
			uint8 count=0;
			do{
				count++;
				RECEIVE_PW( check );        			// RECIEVE PW USER SENDS WE WANT TO CHECK
				VERIFY_PW (P_W,check);					//CHECK IF PW USER SENT IS CORRECT
			} while(Valid == 0 && count<3);

			if(Valid){
				SECONDS_T0_MC2 = 0;
				while ( SECONDS_T0_MC2 <= 15)   { 	MOTOR_clockw;       }
				while ( SECONDS_T0_MC2 <= 18)	{   MOTOR_stop;			}
				while ( SECONDS_T0_MC2 <= 33)   {   MOTOR_anti_clockw;  }
				MOTOR_stop;
			}
			else if(!Valid){
				buzzer_start();
				SECONDS_T0_MC2 = 0;
				while ( SECONDS_T0_MC2 <= 60);
				buzzer_stop();
				break;
			}
		}

	}

}



void RECEIVE_PW(uint8 PW[] )
{
	for (uint8 i = 0; i < 4; i++) {
		PW[i]=UART_receiveByte();
	}
	_delay_ms(DELAY_UART);

}


void VERIFY_PW (uint8 PW[] , uint8 check_pw[])
{

	for (uint8 i = 0; i < 4; i++)
	{
		if (PW[i] != check_pw[i])			//IF ONE CHAR IS DIFFRENT THEN PW IS INVALID
		{
			UART_sendByte(0);
			_delay_ms(DELAY_UART);
			Valid=0;
			break;
		}
		Valid=1;
	}

	if(Valid) {	UART_sendByte(1); _delay_ms(DELAY_UART);}
}
void timer0_isr_fn(void)
{
	quarter_sec++;
	if(quarter_sec==30)
	{
	SECONDS_T0_MC1 ++;
	SECONDS_T0_MC2 ++;
	quarter_sec=0;
	}
}

