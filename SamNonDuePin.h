/*
  Copyright (c) 2011 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
Code from wiring-digital.c and wiring-analog.c from the arduino core.
See undefined.cpp file for more info
*/

#ifndef SAM_NON_DUE_PIN_H
#define SAM_NON_DUE_PIN_H

#include "Arduino.h"

// Number of pins defined in PinDescription array
#define PINS_C 28

//undefined pins constants so the undef pins can
//be refered to a Xn rather than n
static const uint8_t X0  = 0;  //A5
static const uint8_t X1  = 1;  //C27
static const uint8_t X2  = 2;  //A0
static const uint8_t X3  = 3;  //A1
static const uint8_t X4  = 4;  //C11
static const uint8_t X5  = 5;  //PWML3
static const uint8_t X6  = 6;  //PWML0
static const uint8_t X7  = 7;  //PWML2
static const uint8_t PIN_HSMCI_MCCDA_GPIO  = 8;
static const uint8_t PIN_HSMCI_MCCK_GPIO  = 9;
static const uint8_t PIN_HSMCI_MCDA0_GPIO  = 10;
static const uint8_t PIN_HSMCI_MCDA1_GPIO  = 11;
static const uint8_t PIN_HSMCI_MCDA2_GPIO  = 12;
static const uint8_t PIN_HSMCI_MCDA3_GPIO  = 13;
static const uint8_t X8  = 14;   //C20
static const uint8_t X9  = 15;   //B0
static const uint8_t X10  = 16;  //B1
static const uint8_t X11  = 17;  //B2
static const uint8_t X12  = 18;  //B3
static const uint8_t X13  = 19;  //B5
static const uint8_t X14  = 20;  //B6
static const uint8_t X15  = 21;  //A27
static const uint8_t X16  = 22;  //A28
static const uint8_t X17  = 23;  //C29
static const uint8_t X18  = 24;  //C19
static const uint8_t X19  = 25;  //C18
static const uint8_t X20  = 26;  //C17
static const uint8_t X21  = 27;  //C16
static const uint8_t X22  = 28;  //C15


// struct used to hold the descriptions for the "non arduino" pins.
// from the Arduino.h files
extern const PinDescription nonDuePinDescription[] ;
extern void pinModeNonDue( uint32_t ulPin, uint32_t ulMode );
extern void digitalWriteNonDue( uint32_t ulPin, uint32_t ulVal );
extern int digitalReadNonDue( uint32_t ulPin);
extern void analogWriteNonDue(uint32_t ulPin, uint32_t ulValue);
extern void analogOutputNonDue();
extern void hsmciPinsinit();
#endif /* SAM_NON_DUE_PIN_H */


