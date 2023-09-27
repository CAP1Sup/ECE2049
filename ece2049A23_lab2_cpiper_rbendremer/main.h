#pragma once

#include <msp430.h>
#include <peripherals.h>
#include <stdlib.h>


// Function declarations
uint32_t getPrevNoteDuration(uint8_t currentNote);
void initUserLeds();
void initTimerA();
uint32_t getTimerA2Millis();
void resetTimerA2Count();
void initButtons();
uint8_t getPressedButtons();
void initBuzzer();
void displayUserLeds(uint8_t leds);
void displayNote(uint16_t freq);
void turnOffAllOutputs();
void showNote(uint16_t freq);
void playNote(uint16_t freq);
void waitForRestart();
void clearDisplay();
void displayCenteredText(uint8_t* string);
void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t* string3,
                          uint8_t* string4);
bool giveStrike();
void takeAwayStrike();
void displayStrikes();
uint8_t freqToNoteBitGroup(uint16_t freq);
