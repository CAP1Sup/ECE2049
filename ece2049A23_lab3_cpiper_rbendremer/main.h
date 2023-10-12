#pragma once

#include <msp430.h>
#include <peripherals.h>
#include <stdlib.h>

// Function declarations
uint32_t getPrevNoteDuration(uint8_t currentNote);
void initUserLeds();
void initTimerA();
void initADC();
uint32_t getSec();
void initButtons();
void clearDisplay();
void displayCenteredText(uint8_t* string);
void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t* string3,
                          uint8_t* string4);
uint8_t daysToMonths(uint32_t days);
uint8_t remainingDays(uint32_t days, uint8_t month);
void displayDate(uint32_t days);
void displayTime(uint32_t timeInSeconds);