/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 *
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

// Main program for exercise

//****************************************************
//By default, every outputs used in this exercise is 0
//****************************************************
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xgpiops.h"
#include "xttcps.h"
#include "xscugic.h"
#include "xparameters.h"
#include "Pixel.h"
#include "Interrupt_setup.h"



#define enable_interrupts

struct Point {
	uint8_t x;
	uint8_t y;
};

volatile uint8_t channel = 0;
volatile uint8_t* buttonPtr = 0xE000A068;

volatile struct Point enemyLocation = {0, 7};
volatile int8_t enemyDirection = +1;
volatile struct Point ship[4];
volatile struct Point bulletLocation = {0,0};
volatile uint8_t isBulletVisible = 0;
volatile uint8_t points = 0;
volatile uint8_t missedShots = 0;
volatile uint8_t gameIsRunning = 0;

void startGame();
void clearDisplay();
void createCheckerboard();
void setShipPixels(uint8_t, uint8_t, uint8_t);
void initShip();
void moveShip(int8_t);
void updateBullet();
void updateEnemy();
void updatePointsAndMissedShots();
void showPoints();
void endGame();

int main()
{
	    init_platform();


#ifdef	enable_interrupts
	    init_interrupts();
#endif

	    //setup screen
	    setup();

	    Xil_ExceptionEnable();

	    createCheckerboard();

	   	while(1){}

		cleanup_platform();
		return 0;
}



//Interrupt handler for switches and buttons. Connected buttons and switches are at bank2. Reading Status will tell which button or switch was used
void ButtonHandler(void *CallBackRef, u32 Bank, u32 Status)
{
	uint8_t restartButtonPressed = Status & 0b1000;
	uint8_t moveLeftButtonPressed = Status & 0b0010;
	uint8_t moveRightButtonPressed = Status & 0b0001;
	uint8_t shootButtonPressed = Status & 0b0100;

	if(restartButtonPressed){
		startGame();
	} else if(gameIsRunning){
		if(moveRightButtonPressed){
			moveShip(+1);
		} else if(moveLeftButtonPressed){
			moveShip(-1);
		}
		if(shootButtonPressed && !isBulletVisible){
			bulletLocation.x = ship[3].x;
			bulletLocation.y = ship[3].y+1;
			isBulletVisible = 1;
			SetPixel(bulletLocation.x, bulletLocation.y,0xFF,0,0);
		}
	}
}

//Timer interrupt handler for led matrix update. Frequency is 800Hz
void TickHandler(void *CallBackRef){
	//Don't remove this
	uint32_t StatusEvent;

	//exceptions must be disabled when updating screen
	Xil_ExceptionDisable();

	run(channel);
	channel++;
	if(channel>=8)
		channel = 0;

	StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
	XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);
	//enable exeptions
	Xil_ExceptionEnable();
}


//Timer interrupt for moving alien, shooting... Frequency is 10Hz by default
void TickHandler1(void *CallBackRef){

	uint32_t StatusEvent;

	if(gameIsRunning) {
		updateEnemy();
		updateBullet();
		updatePointsAndMissedShots();
	}
	StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
	XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);

}

void startGame(){
	 clearDisplay();
	 initShip();
	 enemyLocation.x = 0;
	 enemyLocation.y = 7;
	 isBulletVisible = 0;
	 points = 0;
	 missedShots = 0;
	 gameIsRunning = 1;
}

void clearDisplay(){
	for(int i = 0; i < 8; ++i){
		for(int j = 0; j < 8; ++j){
			SetPixel(i, j, 0, 0, 0);
		}
	}
}

void createCheckerboard(){
	for(int i = 0; i < 8; ++i){
		for(int j = 0; j < 8; ++j){
			uint8_t red = 0;
			if(i % 2 == 1)
				red = 0xFF;
			uint8_t green = 0;
			if(j % 2 == 1)
				green = 0xFF;
			SetPixel(i, j, red, green, 0xFF);
		}
	}
}

void updateEnemy(){
	uint8_t newX = enemyLocation.x;

	if(enemyDirection > 0){
		if(enemyLocation.x >=7){
			enemyDirection = -1;
			newX--;
		}
		else{
			newX++;
		}
	}
	else{
		if(enemyLocation.x <=0){
			enemyDirection = +1;
			newX++;
		}
		else{
			newX--;
		}
	}
	SetPixel(enemyLocation.x, enemyLocation.y,0,0,0);
	enemyLocation.x = newX;
	SetPixel(enemyLocation.x, enemyLocation.y,0,0,0xFF);
}


void updateBullet(){
	if(isBulletVisible){
		SetPixel(bulletLocation.x, bulletLocation.y,0,0,0);
		bulletLocation.y++;
		if(bulletLocation.y < 8){
			SetPixel(bulletLocation.x, bulletLocation.y,0xFF,0,0);
		}else{
			isBulletVisible = 0;
		}
	}
}

void updatePointsAndMissedShots(){
	if(isBulletVisible){
		if(bulletLocation.y == 7){
			if(bulletLocation.x == enemyLocation.x){
				++points;
				SetPixel(bulletLocation.x, bulletLocation.y,0xFF,0xFF,0);
			} else{
				++missedShots;
				if(missedShots >= 3){
					endGame();
				}
			}
		}
	}
}

void initShip(){
	for(int i = 0; i < 3; ++i)
	{
		ship[i].x = i;
		ship[i].y = 0;
	}
	ship[3].x = 1;
	ship[3].y = 1;

	setShipPixels(0, 0xFF, 0);
}

void setShipPixels(uint8_t r, uint8_t g, uint8_t b){
	for(int i = 0; i < 4; ++i){
		SetPixel(ship[i].x, ship[i].y, r, g, b);
	}
}

void moveShip(int8_t movement){
	for(int i = 0; i < 4; ++i){
		int8_t newX = ship[i].x + movement;
		if(newX < 0 || newX > 7){
			return;
		}
	}
	setShipPixels(0,0,0);
	for(int i = 0; i < 4; ++i){
		ship[i].x += movement;
	}
	setShipPixels(0,0xFF,0);
}

void endGame(){
	setShipPixels(0, 0, 0);
	SetPixel(bulletLocation.x, bulletLocation.y,0,0,0);
	SetPixel(enemyLocation.x, enemyLocation.y,0,0,0);
	gameIsRunning = 0;
	showPoints();
}

void showPoints(){
	for(int bit = 0; bit < 8; ++bit){
		uint8_t mask = 1 << bit;
		uint8_t bitValue = points & mask;
		if(bitValue){
			uint8_t x = 7 - bit;
			SetPixel(x, 7 ,0,0,0xFF);
		}
	}
}

