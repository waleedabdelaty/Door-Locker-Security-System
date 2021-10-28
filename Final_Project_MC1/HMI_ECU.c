/*
 * HMI_ECU.c
 *
 *  Created on: Oct 23, 2021
 *      Author: TOSHIBA PC
 */
#include "timer.h"
#include "lcd.h"
#include "keypad.h"
#include "uart.h"
#include "micro_config.h"

#define DELAY_Keypad 2500
#define DELAY_UART 130

/*******************************************************************************
 *                       Function Prototype                           *
 *******************************************************************************/
void EnterPW(uint8 PW[] );
void SendPW_UART( uint8 PW[] );
void timer0_isr_fn(void);

int quarter_sec=0;

int main(void)
{
	uint8 PW[4];
	uint8 confirm_pw[4];
	uint8 command;
	volatile uint8 check_pw;
	Timer0_setCallBack(timer0_isr_fn);

	LCD_init();
	LCD_clearScreen();

	UART_Config UConfig = {ASYNCH,ENABLED_EVEN,BIT_1};// Asynchronous with even parity and only one stop bit
	UART_init(&UConfig);

	TimerConfig T0_Configuration={NORMAL,F_CPU_1024,0,7812};  // TIMER0 CONFIGURATIONS
	Timer0_init(&T0_Configuration); 			 //Initialize timer0
	SREG |= (1<<7);

	/*******************************************************************************
	 *                       For First Time                          *
	 *******************************************************************************/
do{
	//                                      ENTERING PASSWORD
	LCD_clearScreen();
	LCD_displayString("Enter Password");
	EnterPW( PW );            					 // SEND ADDRESS OF password[4] TO EnterPW()
	_delay_ms(DELAY_UART);
	SendPW_UART ( PW );
	// 									RE-ENTERING PASSWORD
	LCD_clearScreen();
	LCD_displayString("RE-Enter PW");
	EnterPW(confirm_pw);
	_delay_ms(DELAY_UART);
	SendPW_UART ( confirm_pw );
	//									Verifying Password
	//+++++++++++++++++++ MCU2 will check if password ok -> 1 and if not ->0
	check_pw = UART_receiveByte();
	// send message to user if not valid
	if(check_pw==0)
	{
		LCD_clearScreen();
		LCD_displayStringRowColumn(0,4,"INVALID");
		_delay_ms(DELAY_Keypad);
	}
	// if password okay get out from do-while loop
}while (check_pw== 0);
	LCD_clearScreen();
	LCD_displayString("Correct");
	_delay_ms(DELAY_Keypad);

	/*******************************************************************************
		 *                      Change password                         *
		 *******************************************************************************/
	while(1)
	  {
	LCD_clearScreen();
	LCD_displayStringRowColumn(0, 0, "- :Change PW");
	LCD_displayStringRowColumn(1, 0, "+ :open");

	// save user choice and then send it to MC2
	command=KEYPAD_getPressedKey();
	_delay_ms(DELAY_Keypad);
	UART_sendByte(command);
	_delay_ms(DELAY_UART);

	//+++++++++++++++++++++++ Change PW (press - ) ++++++++++++++++++++++++++++


	if (command == '-')
	        {
	        	do{
	        		//                                      ENTERING PASSWORD

	            LCD_clearScreen();
	        	LCD_displayString("Enter Password");
	        	EnterPW( PW );            					 // SEND ADDRESS OF password[4] TO EnterPW()
	        	SendPW_UART ( PW );						 	 // SEND PW TO MC2 via UART

	        	// 									RE-ENTERING PASSWORD

	            LCD_clearScreen();
	        	LCD_displayString("Re-enter PW");
	        	EnterPW( confirm_pw );
	         	SendPW_UART ( confirm_pw );
	        	//									Verifying Password
	        	//+++++++++++++++++++ MCU2 will check if password ok -> 1 and if not ->0
	         	check_pw=UART_receiveByte();
	         	_delay_ms(DELAY_UART);

	         	if (check_pw == 0)
	         	{
	         		LCD_clearScreen();
	         		LCD_displayStringRowColumn(0,4,"INVALID");
	         		_delay_ms(DELAY_Keypad);
	         	}

	            }while (check_pw == 0);

	            LCD_clearScreen();
	        	LCD_displayString("Password Changed");
	            _delay_ms(DELAY_Keypad);
	        }
	//+++++++++++++++++++++++ open  (press + ) ++++++++++++++++++++++++++++
	else if(command == '+'){

		// here we need to check if pass matches what the user saved in EEPROM
		uint8 count=0;

		do{
			 count++;
			 //                                      ENTERING PASSWORD
			 LCD_clearScreen();
			 LCD_displayString("Enter Password");
			 EnterPW( PW );            					 // SEND ADDRESS OF password[4] TO EnterPW()
			 SendPW_UART ( PW );						 	 // SEND PW TO MC2 via UART
			 //									Verifying Password
			 //+++++++++++++++++++ MCU2 will check if password ok -> 1 and if not ->0
			 check_pw=UART_receiveByte();
			 _delay_ms(DELAY_UART);

			 if (check_pw == 0)
			 {
			     LCD_clearScreen();
			     LCD_displayStringRowColumn(0,4,"INVALID");
			     _delay_ms(DELAY_Keypad);
			 }

		}while(check_pw==0 && count <3); // out check here if count greater than 3 it will disable lcd for a while
		if(check_pw)
		{
			/* DOOR OPENS IN 15 SECONDS AND STAYS OPENED FOR 3 SECONDS AND STARTS
			    				 *  CLOSING AGAIN IN 15 SECONDS */

			SECONDS_T0_MC1 = 0;
			LCD_clearScreen(); LCD_displayString("Opening Door");
			while ( SECONDS_T0_MC1 <= 15);
			LCD_clearScreen(); LCD_displayString("Door Open");
			while ( SECONDS_T0_MC1 <= 18);
			LCD_clearScreen(); LCD_displayString("Closing Door");
			while ( SECONDS_T0_MC1 <= 33);
		}
		// if password do not match so turn on buzzer
		else if(check_pw==0)
		{
		   LCD_clearScreen();
		   LCD_displayStringRowColumn(0,5,"ALARM");
		   SECONDS_T0_MC1 = 0;
		   while ( SECONDS_T0_MC1 < 60);       // LOCK MC1 FOR 60 SECONDS
		}
	  }
	}

}

void EnterPW(uint8 PW[] )
{
	LCD_moveCursor(1, 5);
	for (uint8 i = 0; i < 5; i++) {
		while ((KEYPAD_getPressedKey() != 13) && (i == 4));  // POLLING UNTIL ON BUTTON PRESSED

		PW[i] = KEYPAD_getPressedKey();
		_delay_ms(DELAY_Keypad);				//DELAY SO KEYPAD DOESN'T REPEAT
		LCD_displayCharacter('*');
	}
	_delay_ms(DELAY_Keypad);
}


void SendPW_UART( uint8 PW[] )
{
		for (uint8 i=0 ; i<4 ; i++)
		{
			UART_sendByte( PW[i] );
			_delay_ms(DELAY_UART);
		}

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
