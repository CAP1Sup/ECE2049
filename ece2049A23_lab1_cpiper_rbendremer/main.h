#pragma once

// Function declarations
void initLEDs();
void initButtons();
void initBuzzer();
void showNum(uint8_t num);
void buzzerSound(uint8_t num);
uint8_t getPressedButtons();
void waitForRestart();
void clearDisplay();
void displayCenteredText(uint8_t* string);
void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t* string3);
void lose();
uint8_t buttonToNum(uint8_t buttonStates);
void delayCycles(uint32_t cycles);
