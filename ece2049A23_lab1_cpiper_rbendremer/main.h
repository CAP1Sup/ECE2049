#pragma once

// Function declarations
void initLEDs();
void initButtons();
void initBuzzer();
void showNum(uint8_t num);
void buzzerSound(uint8_t num);
uint8_t getPressedButtons();
void waitForRestart();
void displayCenteredText(uint8_t* string);
void lose();
uint8_t* numToString(uint8_t num);
