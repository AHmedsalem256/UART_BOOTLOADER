/*
 * main.c
 *
 *  Created on: Oct 2, 2023
 *      Author: Ahmed Salem
 */


#include "STD_TYPES.h"
#include "BIT_MATH.h"

#include "RCC_interface.h"
#include "DIO_interface.h"
#include "STK_interface.h"
#include "USART_interface.h"
#include "FPEC_interface.h"

void Parser_voidParseRecord(u8* Copy_u8BufData);


#define STK_DELAY_TIME   5000000

volatile u8  u8RecBuffer[100]   ;  // Buffer array to recieve the data from UART
volatile u8  u8RecCounter    = 0;
volatile u8  u8TimeOutFlag   = 0;
volatile u16 u16TimerCounter = 0;  //
volatile u8  u8BLWriteReq    = 1;  //Used as flag to write on flash memory


typedef void (*Function_t)(void);

Function_t addr_to_call = 0;

void func(void)
{
	// used to guid processor to fetch instruction starting from new application address
	// Application is stored in Flash memory

	#define SCB_VTOR   *((volatile u32*)0xE000ED08)

	/* Guiding the {PC} to start from specific address */

	SCB_VTOR = 0x08001000;

	addr_to_call = *(Function_t*)(0x08001004);
	addr_to_call();
}


void main(void)
{

	RCC_voidInitSysClock();

	/* USART1 -> Clock */
	RCC_voidEnableClock(RCC_APB2,14);

	/* PORTA -> CLOCK*/
	RCC_voidEnableClock(RCC_APB2,2);

	/* FPEC -> Clock */
	RCC_voidEnableClock(RCC_AHB,4);   /* FPEC   */


	MGPIO_VidSetPinDirection(GPIOA,9,0b1010);   /* TX AFPP */
	MGPIO_VidSetPinDirection(GPIOA,10,0b0100);  /* RC Input Floating */

	MGPIO_VidSetPinDirection(GPIOA,0,0b0001);
	MGPIO_VidSetPinValue(GPIOA,0,HIGH);

	/* Starting the bootloader */
	u8 Local_u8RecStatus;

	MUSART1_voidInit();

	MSTK_voidInit();

	// Starting 5 sec to check for recieve Records from UART_1
	MSTK_voidSetIntervalSingle(STK_DELAY_TIME,func);

	while(u8TimeOutFlag == 0) // I used u8TimeOutFlag for future processes
	{

		Local_u8RecStatus = MUSART1_u8Receive( &(u8RecBuffer[u8RecCounter]) );

		if (Local_u8RecStatus == 1)
		{
			MSTK_voidStopInterval();

			if(u8RecBuffer[u8RecCounter] == '\n')
			{
				if (u8BLWriteReq == 1)
				{
					/* The flash should be erased only once to write on the erased address */
					FPEC_voidEraseAppArea();
					u8BLWriteReq = 0; // To make sure that flash is erased only once
				}
				
				/* Start parsing the new records */
				Parser_voidParseRecord(u8RecBuffer);

				// As the tool that sends the Hex records rewuires "ok" massege to send the reccords
				MUSART1_voidTransmit("ok");

				/* To start recieving from first of the data buffer */
				u8RecCounter = 0;
			}

			else
			{
				u8RecCounter ++ ;
			}

			MSTK_voidSetIntervalSingle(STK_DELAY_TIME,func);
		}

		else
		{

		}
	}
}
