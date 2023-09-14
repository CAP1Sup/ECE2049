#pragma once

// Function declarations
void initTimerA();
void initButtons();
void initBuzzer();
void showNote(uint16_t freq);
void buzzerSound(uint16_t freq);
uint8_t getPressedButtons();
void waitForRestart();
void clearDisplay();
void displayCenteredText(uint8_t* string);
void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t* string3);
uint8_t noteToNum(uint16_t freq);
uint8_t buttonToNum(uint8_t buttonStates);
void delayCycles(uint32_t cycles);
