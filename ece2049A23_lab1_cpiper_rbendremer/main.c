#include <msp430.h>
#include <peripherals.h>
#include <stdint.h>
#include <stdlib.h>
#include <main.h>

// Settings
#define PLAYBACK_ON_DELAY 1000000
#define PLAYBACK_OFF_DELAY 100000
#define LOSE_DELAY 1000000
#define COUNTDOWN_DELAY 1000000
#define MAX_BUTTON_CHECKS 10000
#define KEY_DEBOUNCE_DELAY 100

// Variables
uint8_t numList[50];  // Would prefer uint4_t for nums
uint8_t seqLen = 0;

// Current state
enum State { WELCOME, PLAYBACK, INPUT, WINNER };
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

        // Do a count down
        uint8_t i;
        for (i = 3; i > 0; i--) {
          // Display the number
          displayCenteredText(numToString(i));

          // Wait for a second
          __delay_cycles(COUNTDOWN_DELAY);
        }

        // Move to the playback state
        currState = PLAYBACK;
        break;
      }
      case PLAYBACK: {
        // Generate a random number to add to the sequence
        numList[seqLen] = 1 << rand() % 4;

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
        // Watch for input
        uint8_t currIndex = 0;
        uint32_t buttonChecks = 0;

        // Loop through the sequence
        for (currIndex = 0; currIndex < seqLen; currIndex++) {
          // Get the pressed buttons
          uint8_t pressed = getPressedButtons();

          // If a button is pressed
          if (pressed) {
            // Show the pressed number on the display
            Graphics_clearDisplay(&g_sContext);
            Graphics_drawStringCentered(&g_sContext, numToString(pressed),
                                        AUTO_STRING_LENGTH, pressed * 15, 15,
                                        TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            // Check if the button pressed is the correct one
            if (pressed == numList[currIndex]) {
              // Reset the button checks
              buttonChecks = 0;

              // Move to the next number
              continue;
            } else {
              // Wrong button pressed
              lose();
              break;
            }

            // Delay (to prevent debouncing), then to allow user to release button
            __delay_cycles(KEY_DEBOUNCE_DELAY);
            while (getPressedButtons());
            __delay_cycles(KEY_DEBOUNCE_DELAY);
          }

          // Check if the user took too long
          if (buttonChecks > MAX_BUTTON_CHECKS) {
            // Time up
            lose();
            break;
          }

          // Increment the button checks
          buttonChecks++;
        }
        break;
      }
      case WINNER: {
        // Tell the user that they won :)
        displayCenteredText("You won!");
        waitForRestart();
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

  // Enable pull up resistors
  P7REN |= (BIT0 | BIT4);
  P3REN |= (BIT6);
  P2REN |= (BIT2);
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
  setLeds(1 << num);
  buzzerSound(num);
  __delay_cycles(PLAYBACK_ON_DELAY);
  setLeds(0);
  BuzzerOff();
  __delay_cycles(PLAYBACK_OFF_DELAY);
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
 * @brief Waits for the user to press the * key, then restart the game
 *
 */
void waitForRestart() {
  // Wait for the * key to be pressed to restart
  while (!(getKey() == '*'))
    ;

  // Reset the sequence length
  seqLen = 0;

  // Move to the playback state
  currState = PLAYBACK;
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
 * @brief Makes a losing sound and moves to the welcome screen
 *
 */
void lose() {
  buzzerSound(0);
  __delay_cycles(LOSE_DELAY);
  BuzzerOff();

  // Move to the welcome screen
  currState = WELCOME;
}

/**
 * @brief Converts the given number to a string
 *
 * @param num Number to convert
 * @return uint8_t* String pointer
 */
uint8_t* numToString(uint8_t num) {
  uint8_t* string = malloc(sizeof(uint8_t) * 2);
  string[0] = '0' + num;
  string[1] = '\0';
  return string;
}
