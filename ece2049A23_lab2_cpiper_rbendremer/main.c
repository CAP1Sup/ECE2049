#include <msp430.h>
#include <peripherals.h>

// Undefine C (so we can use it as a note)
#ifdef C
#undef C
#endif

// Note definitions
#define A 440
#define Bb 466
#define B 494
#define C 523
#define Cs 554
#define D 587
#define Eb 622
#define E 659
#define F 698
#define Fs 740
#define G 784
#define Ab 831
#define A_H 880

// Function Prototypes

// Declare globals here
uint16_t notes[30] = {A, Bb, B,  C, Cs, D, Eb, E,  F, Fs, G,   Ab, A_H, A, Bb,
                     B, C,  Cs, D, Eb, E, F,  Fs, G, Ab, A_H, A,  Bb,  B, C};

// State
enum State { WELCOME, PLAYING, LOSER, WINNER };
enum State currState = WELCOME;

// Main
void main(void) {
  WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer. Always need to stop this!!
                             // You can then configure it properly, if desired

  // Init peripherals (timer for buzzer, buttons, etc.)
  initLeds();
  initButtons();
  initTimerA();
  initBuzzer();
  configDisplay();
  configKeypad();

  // Main loop
  while (1) {
    switch (currState) {
      case WELCOME: {
        // Display MSP430 Hero on screen

        // Do a count down

        // Move to the playing state
        currState = PLAYING;
        break;
      }

      case PLAYING: {
        // Loop through the sequence

        // Watch for input

        // Check if the game needs restarted

        // Get the pressed button(s) and check them against the sequence

        // Check if the user took too long

        // Give user a strike if needed

        // User loses if too many strikes

        // If the user was able to repeat the pattern successfully, move back
        // to displaying the numbers
        currState = WINNER;
        break;
      }
      case LOSER: {
        // Tell the user that they lost :(

        // Wait for a button press to restart the game

        currState = WELCOME;
        break;
      }
      case WINNER: {
        // Tell the user that they won :)

        // Wait for a button press to restart the game

        currState = WELCOME;
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
 * @brief Initializes timer A
 *
 */
void initTimerA() {
  // Configure Timer A2 to use ACLK, divide by 1, continuous counting mode
  TA2CTL = (TASSEL__ACLK | ID__1 | MC__CONTINUOUS);
  TA2CTL &= ~TAIE;  // Explicitly disable timer interrupts for safety
  TB0CCTL0 &= ~CCIE;  // Disable timer interrupts

  // Timer period in ticks
  TA2CCR0 = 32768 / 1000;  // 32768 ticks per second, so this is a period of 1
                           // milliseconds.

  // Disable both capture/compare periods (buzzer shouldn't be on yet)
  TB0CCTL0 = 0;
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
 * @param num The frequency to sound the buzzer at
 */
void buzzerSound(uint16_t freq) {
  // Now configure the timer period, which controls the PWM period
  // Doing this with a hard coded values is NOT the best method
  // We do it here only as an example. You will fix this in Lab 2.
  //TB0CCR0 = (5 - num) * 32;  // Set the PWM period in ACLK ticks
  TB0CCTL0 &= ~CCIE;         // Disable timer interrupts

  // Configure CC register 5, which is connected to our PWM pin TB0.5
  TB0CCTL5 = OUTMOD_3;    // Set/reset mode for PWM
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
