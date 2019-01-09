/*
 * Pixel.c


 *
 *  Created on: -----
 *      Author: -----
 */

#define COLUMN_LENGTH 8
#define COLOR_COUNT 3

#include "Pixel.h"

//Table for pixel dots. dots[page][X][Y][COLOR]
volatile uint8_t dots[Page_size][8][8][3]={0};
volatile uint8_t page=0;
volatile uint8_t* control_ptr = 0x41220008;
volatile uint8_t* channel_ptr = 0x41220000;
volatile uint8_t* led_ptr = 0x41200000;

void setup(){
	*control_ptr = 0x00;
	//Reset
	*control_ptr |= 0b00000001;


	//Write code that set gamma values to led matrix driver (6-bit data)

	// Set SDA to high
	*control_ptr |= 0b00010000;
	for(int i = 0; i < (COLUMN_LENGTH * COLOR_COUNT * 6); ++i)
	{
		doSckPulse();
	}
	// Set SB to 8-bit bank
	*control_ptr |= 0b00000100;
}

//Set value of one pixel at led matrix
void SetPixel(uint8_t x,uint8_t y, uint8_t r, uint8_t g, uint8_t b){
	dots[page][x][y][0]=r;
	dots[page][x][y][1]=g;
	dots[page][x][y][2]=b;
}


//Put new data to led matrix
void run(uint8_t c){

	//check if page exist
	if (page>=Page_size) return;

	for(int row = 7; row >= 0; --row)
	{
		for(int color = 2; color >= 0; --color)
		{
			uint8_t brightness = dots[page][c][row][color];
			for(int bit = 7; bit >= 0; --bit)
			{
				uint8_t mask = 1 << bit;
				uint8_t bitValue = brightness & mask;
				setSda(bitValue);
				doSckPulse();
			}
		}
	}

	open_line(c);
	latch();
}

//latch signal.
void latch(){
	*control_ptr |= 0b00000010;
	*control_ptr &= 0b11111101;
}


//Set one line as active per time.
void open_line(uint8_t i){
	*channel_ptr = 1 << i;
}

void doSckPulse(){
	*control_ptr &= 0b11110111;
	*control_ptr |= 0b00001000;
}

void setSda(uint8_t value){
	if(value == 0){
		*control_ptr &= 0b11101111;
	}
	else{
		*control_ptr |= 0b00010000;
	}
}
