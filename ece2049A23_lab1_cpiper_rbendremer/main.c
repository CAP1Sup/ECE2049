#include <msp430.h>
#include <peripherals.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <main.h>

// Settings (delays are in CPU cycles)
#define PLAYBACK_ON_DELAY 100000
#define PLAYBACK_OFF_DELAY 10000
#define LOSE_DELAY 1000000
#define COUNTDOWN_DELAY 1000000
#define MAX_BUTTON_CHECKS 50000
#define NUM_DISPLAY_CHECKS 10000
#define KEY_DEBOUNCE_DELAY 10000
#define NUM_DISPLAY_X_OFFSET 0
#define NUM_DISPLAY_X_MOVE 25
#define SPEEDUP_FACTOR 10 // Factor of reduction in time

// Variables
uint8_t numList[50];  // Would prefer uint4_t for nums
uint8_t seqLen = 0;

// Current state
enum State { WELCOME, PLAYBACK, INPUT };
enum State currState = WELCOME;

/**
 * main.c
 */
int main(void) {
  WDTCTL = WDTPW | WDTHOLD;  // stop watchdog timer

  // Init peripherals
  initLeds();
  initButtons();
  initBuzzer();
  configDisplay();
  configKeypad();

  // Main loop
  while (1) {
    switch (currState) {
      case WELCOME: {
        // Display SIMON on screen
        displayCenteredText("SIMON");

        // Wait for the * key to be pressed
        while (!(getKey() == '*'))
          ;

        // Reset the seq length
        seqLen = 0;

        // Do a count down
        displayCenteredText("3");
        __delay_cycles(COUNTDOWN_DELAY);
        displayCenteredText("2");
        __delay_cycles(COUNTDOWN_DELAY);
        displayCenteredText("1");
        __delay_cycles(COUNTDOWN_DELAY);

        // Move to the playback state
        currState = PLAYBACK;
        break;
      }
      case PLAYBACK: {
        displayCenteredTexts("Memorize", "the", "pattern");

        // Generate a random number to add to the sequence
        numList[seqLen] = rand() % 4;

        // Increment the sequence length
        seqLen++;

        // Display the numbers one by one
        uint8_t i;
        for (i = 0; i < seqLen; i++) {
          showNum(numList[i]);
        }

        // Switch to the input state
        currState = INPUT;
        break;
      }
      case INPUT: {
        displayCenteredTexts("Repeat", "the", "pattern");

        // Watch for input
        uint8_t currIndex = 0;
        uint32_t buttonChecks = 0;

        // Loop through the sequence
        for (currIndex = 0; currIndex < seqLen; currIndex++) {
          while (currState == INPUT) {
              // Check if the game needs restarted
              if (getKey() == '#') {
                 currState = WELCOME;
                 break;
              }

              // Get the pressed buttons
              uint8_t pressed = getPressedButtons();

              // If a button is pressed
              if (pressed) {

                // Show the pressed number on the display
                Graphics_clearDisplay(&g_sContext);
                switch(buttonToNum(pressed)) {
                    case 1: {
                        Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH,
                                                    NUM_DISPLAY_X_OFFSET, 15, TRANSPARENT_TEXT);
                        break;
                    }
                    case 2: {
                        Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH,
                                                    NUM_DISPLAY_X_OFFSET + NUM_DISPLAY_X_MOVE, 15, TRANSPARENT_TEXT);
                        break;
                    }
                    case 3: {
                        Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH,
                                                    NUM_DISPLAY_X_OFFSET + NUM_DISPLAY_X_MOVE*2, 15, TRANSPARENT_TEXT);
                        break;
                    }
                    case 4: {
                        Graphics_drawStringCentered(&g_sContext, "4", AUTO_STRING_LENGTH,
                                                    NUM_DISPLAY_X_OFFSET + NUM_DISPLAY_X_MOVE*3, 15, TRANSPARENT_TEXT);
                        break;
                    }
                }
                Graphics_flushBuffer(&g_sContext);

                // Check if the button pressed is the correct one
                if (pressed == (1 << numList[currIndex])) {
                  // Reset the button checks
                  buttonChecks = 0;

                  // Delay to prevent button debouncing, then wait for the button to be released
                  __delay_cycles(KEY_DEBOUNCE_DELAY);
                  while (getPressedButtons());
                  __delay_cycles(KEY_DEBOUNCE_DELAY);

                  // Move to the next number
                  break;
                } else {
                  // Wrong button pressed
                  lose();
                  break;
                }
              }

              // Check if the user took too long
              if (buttonChecks > (MAX_BUTTON_CHECKS - ((MAX_BUTTON_CHECKS/SPEEDUP_FACTOR) * (seqLen - 1)))) {
                // Time up
                lose();
                break;
              }
              // Clear the display after a specified count
              //if (buttonChecks > NUM_DISPLAY_CHECKS) {
              //    clearDisplay();
              //}

              // Increment the button checks
              buttonChecks++;
          }
        }

        // The user was able to repeat the pattern successfully, move back to displaying the numbers
        if (currState == INPUT) {
            currState = PLAYBACK;
        }
        break;
      }
    }
  }

  return 0;
}

/**
 * @brief Initializes the buttons on the board
 *
 */
void initButtons() {
  // Buttons are on: S1: P7.0, S2: P3.6, S3: P2.2, S4: P7.4

  // Set pins to be digital IO
  P7SEL &= ~(BIT0 | BIT4);
  P3SEL &= ~(BIT6);
  P2SEL &= ~(BIT2);

  // Set pins to be inputs
  P7DIR &= ~(BIT0 | BIT4);
  P3DIR &= ~(BIT6);
  P2DIR &= ~(BIT2);

  // Set internal resistors to pull-ups
  P7OUT |= (BIT0 | BIT4);
  P3OUT |= BIT6;
  P2OUT |= BIT2;

  // Enable pull-up/down resistors
  P7REN |= (BIT0 | BIT4);
  P3REN |= BIT6;
  P2REN |= BIT2;
}

/**
 * @brief Initializes the buzzer
 *
 */
void initBuzzer() {
  // Initialize PWM output on P3.5, which corresponds to TB0.5
  P3SEL |= BIT5;  // Select peripheral output mode for P3.5
  P3DIR |= BIT5;

  TB0CTL = (TBSSEL__ACLK | ID__1 |
            MC__UP);  // Configure Timer B0 to use ACLK, divide by 1, up mode
  TB0CTL &= ~TBIE;    // Explicitly Disable timer interrupts for safety

  // Disable both capture/compare periods (buzzer shouldn't be on yet)
  TB0CCTL0 = 0;
  TB0CCTL5 = 0;
}

/**
 * @brief Displays the given number on the LEDs and sounds the buzzer
 *
 * @param num The number to display (0-3)
 */
void showNum(uint8_t num) {
  setLeds(0b1000 >> num); // Led numbering is reversed :(
  buzzerSound(num);
  delayCycles(PLAYBACK_ON_DELAY - ((PLAYBACK_ON_DELAY/SPEEDUP_FACTOR) * (seqLen - 1)));
  setLeds(0);
  BuzzerOff();
  delayCycles(PLAYBACK_OFF_DELAY - ((PLAYBACK_OFF_DELAY/SPEEDUP_FACTOR) * (seqLen - 1)));
}

/**
 * @brief Sounds the buzzer at different frequencies
 *
 * @param num The number frequency to sound the buzzer at (0-3)
 */
void buzzerSound(uint8_t num) {
  // Now configure the timer period, which controls the PWM period
  // Doing this with a hard coded values is NOT the best method
  // We do it here only as an example. You will fix this in Lab 2.
  TB0CCR0 = (5 - num) * 32;  // Set the PWM period in ACLK ticks
  TB0CCTL0 &= ~CCIE;         // Disable timer interrupts

  // Configure CC register 5, which is connected to our PWM pin TB0.5
  TB0CCTL5 = OUTMOD_7;    // Set/reset mode for PWM
  TB0CCTL5 &= ~CCIE;      // Disable capture/compare interrupts
  TB0CCR5 = TB0CCR0 / 2;  // Configure a 50% duty cycle
}

/**
 * @brief Gets the currently pressed buttons
 *
 * @return uint8_t First 4 bits represent if the buttons pressed
 */
uint8_t getPressedButtons() {
  uint8_t pressed = 0;

  // Check if the buttons are pressed
  if (!(P7IN & BIT0)) {
    pressed |= BIT0;
  }
  if (!(P3IN & BIT6)) {
    pressed |= BIT1;
  }
  if (!(P2IN & BIT2)) {
    pressed |= BIT2;
  }
  if (!(P7IN & BIT4)) {
    pressed |= BIT3;
  }

  return pressed;
}

/**
 * @brief Clears the screen
 */
void clearDisplay() {
    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);
}

/**
 * @brief Displays the given string in the center of the screen
 *
 * @param string The string to display
 */
void displayCenteredText(uint8_t* string) {
  Graphics_clearDisplay(&g_sContext);
  Graphics_drawStringCentered(&g_sContext, string, AUTO_STRING_LENGTH, 48, 15,
                              TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
}

/**
 * @brief Displays the given strings in the center of the screen
 *
 * @param string1 The first string to display
 * @param string2 The second string to display
 * @param string3 The third string to display
 */
void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t* string3) {
  Graphics_clearDisplay(&g_sContext);
  Graphics_drawStringCentered(&g_sContext, string1, AUTO_STRING_LENGTH, 48, 15,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, string2, AUTO_STRING_LENGTH, 48, 30,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, string3, AUTO_STRING_LENGTH, 48, 45,
                              TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
}

/**
 * @brief Makes a losing sound and moves to the welcome screen
 *
 */
void lose() {
  // Display the losing message
  displayCenteredText("You lose!");
  buzzerSound(0);
  __delay_cycles(LOSE_DELAY);
  BuzzerOff();

  // Move to the welcome screen
  currState = WELCOME;
}

/**
 * @brief Converts the given buttons pressed to a number. Assumes that only one button is pressed.
 *
 * @param buttonStates Button states
 * @return uint8_t Rightmost button number pressed
 */
uint8_t buttonToNum(uint8_t buttonStates) {
  uint8_t rightMostPosition = 0;
  while (buttonStates != 0) {
      buttonStates >>= 1;
      rightMostPosition++;
  }
  return rightMostPosition;
}

/**
 * @brief Delays the CPU by the given number of cycles
 *
 * @param cycles The number of cycles to delay by
 */
void delayCycles(uint32_t cycles) {
    // Divide by 2
    // One op for compare, one for subtraction
    cycles /= 2;
    while (cycles) {
        cycles--;
    }
}
