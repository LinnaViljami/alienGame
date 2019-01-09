/*
 * Pixel.h
 *
 *  Created on: ------
 *      Author: ------
 */

#ifndef PIXEL_H_
#define PIXEL_H_

#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xgpiops.h"
#include "xttcps.h"
#include "xscugic.h"
#include "xparameters.h"

//size can be changed if needed
#define Page_size 10


void setup();
void SetPixel(uint8_t x,uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void run(uint8_t c);
void latch();
void open_line(uint8_t i);
void doSckPulse();
void setSda(uint8_t value);

#endif /* PIXEL_H_ */
