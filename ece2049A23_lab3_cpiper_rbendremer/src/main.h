#pragma once

#include <msp430.h>
#include <stdlib.h>

#include "peripherals.h"

// Temperature Sensor Calibration = Reading at 30 degrees C is stored at addr
// 1A1Ah See end of datasheet for TLV table memory mapping
#define CALADC12_15V_30C *((unsigned int*)0x1A1A)
// Temperature Sensor Calibration = Reading at 85 degrees C is stored at addr
// 1A1Ch
// See device datasheet for TLV table memory mapping
#define CALADC12_15V_85C *((unsigned int*)0x1A1C)

// Function declarations
void initButtons();
bool isLeftButtonPressed();
bool isRightButtonPressed();
void initTimerA();
void initADC();
void setupADCContTemp();
float getTempCAvg();
float C_to_F(float tempC);
void setupADCPot();
uint16_t getPot();
uint32_t getSec();
void setTimerA2Count(uint8_t month, uint8_t day, uint8_t hour, uint8_t minute,
                     uint8_t second);
void setTimerA2CountSec(uint32_t seconds);
void clearDisplay();
void displayCenteredText(char* string);
void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t* string3,
                          uint8_t* string4);
uint8_t daysToMonth(uint32_t days);
uint8_t remainingDays(uint32_t days);
uint16_t monthToDays(uint8_t month);
void displayDate(uint32_t days);
void displayTime(uint32_t timeInSeconds);
void displayTempC(float averageTempC);
void displayTempF(float averageTempF);
