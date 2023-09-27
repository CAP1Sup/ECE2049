#include <main.h>

// Undefine C (so we can use it as a note)
#ifdef C
#undef C
#endif

// Note definitions
#define STRIKE_FREQ 200
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

// Note minimum grouping thresholds
#define NOTEG0_MIN B   // Any note below this will be in group 0
#define NOTEG1_MIN D   // Any note between C-D wil be in group 1
#define NOTEG2_MIN Fs  // Any note between Eb-Fs will be in group 2
// No need for a group 3 minimum threshold
// Any note above Fs will be in group 3

// Note/LED deadtime
// Allows the user to see the difference between notes
#define NOTE_DEADTIME 100 // ms
#define LAST_STRIKE_DURATION 1000 // ms
#define LAST_STRIKE_NOTE1 B // Hz
#define LAST_STRIKE_NOTE2 Bb // Hz
#define LAST_STRIKE_NOTE3 A // Hz

// Declare globals here
#define NUM_NOTES 30
// in Hz
uint16_t notes[NUM_NOTES] = {A, Bb, B,   C, Cs, D,   Eb, E,  F, Fs,
                             G, Ab, A_H, A, Bb, B,   C,  Cs, D, Eb,
                             E, F,  Fs,  G, Ab, A_H, A,  Bb, B, C};

// in ms
uint16_t durations[NUM_NOTES] = {2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000,
                 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000,
                 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000};

// Counter ticks every 0.5 ms
// Overflow every (0.5 ms * (2^32 - 1)) = 2,147,483.6475 seconds
uint32_t A2Count = 0;

// Game status info
uint8_t strikes = 0;
uint8_t prevPressedButtons = 0;
bool doublePressed = false;

// State
enum State { WELCOME, PLAYING, LOSER, WINNER };
enum State currState = WELCOME;

// Main
void main(void) {
  WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer. Always need to stop this!!
                             // You can then configure it properly, if desired

  // Enable global interrupts
  _BIS_SR(GIE);

  // Init peripherals (timer for buzzer, buttons, etc.)
  initLeds();
  initUserLeds();
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
        displayCenteredTexts("MSP430", "Hero", "", "Press *");

        // Wait for a button press to start the game
        while (getKey() != '*')
          ;

        // Reset the timer
        resetTimerA2Count();

        // Do a count down
        // 3
        displayCenteredText("3");
        displayUserLeds(0b01);
        while (getTimerA2Millis() < 1000)
          ;

        // 2
        displayCenteredText("2");
        displayUserLeds(0b10);
        while (getTimerA2Millis() < 2000)
          ;

        // 1
        displayCenteredText("1");
        displayUserLeds(0b01);
        while (getTimerA2Millis() < 3000)
          ;

        // Go!
        displayCenteredText("Go!");
        displayUserLeds(0b11);
        while (getTimerA2Millis() < 4000)
          ;

        // Clean up outputs
        turnOffAllOutputs();

        // Display the current number of strikes
        displayStrikes();

        // Move to the playing state
        currState = PLAYING;
        break;
      }

      case PLAYING: {

        // Reset global variables
        strikes = 0;
        prevPressedButtons = 0;
        doublePressed = false;

        // Correct button pressed
        bool correctButtonPressed = false;

        // Initialize the sequence
        uint8_t currentNote = 0;

        // Show the user the first note
        showNote(notes[currentNote]);

        // Loop through the sequence
        while (currentNote < NUM_NOTES) {
          // Check if previous note is done playing
          // Uses first note's duration for the first note's press period
          if (getTimerA2Millis() > getPrevNoteDuration(currentNote) + NOTE_DEADTIME) {

            // Turn off buzzer and note display LEDs
            BuzzerOff();
            setLeds(0b0000);

            // Delay for a bit
            // Allows the user to see the difference between notes
            resetTimerA2Count();
            while (getTimerA2Millis() < NOTE_DEADTIME)
              ;

            // Check if the user needs to be given a strike
            if (!correctButtonPressed) {
              // Play a note to indicate that the user got a strike
              playNote(STRIKE_FREQ);

              if (giveStrike()) {
                break;
              }
            }
            else {
              // Play the note
              playNote(notes[currentNote]);

              // Make sure that the user hasn't double pressed the correct button
              if (!doublePressed) {
                takeAwayStrike();
              }

              correctButtonPressed = false;
              doublePressed = false;
            }

            // Increment the note
            currentNote++;

            // Show the next note
            showNote(notes[currentNote]);
          }

          // Get the state of the input buttons
          uint8_t pressed = getPressedButtons();

          // Check if the user pressed a button
          if (pressed) {
            // Only update if buttons have changed
            if (pressed != prevPressedButtons) {
              // Check if the user pressed the correct button
              if (pressed == freqToNoteBitGroup(notes[currentNote])) {
                // User pressed the correct button
                // Check if the correct button was already pressed
                if (correctButtonPressed) {
                  // User pressed the correct button again
                  // Give them a strike
                  if (giveStrike()) {
                    break;
                  }

                  // Note that the user double pressed the correct button
                  doublePressed = true;
                }
                else {
                  // Note that the correct button has been pushed
                  correctButtonPressed = true;
                }
              } else {
                // User pressed the wrong button
                if (giveStrike()) {
                  break;
                }
              }
            }
          }

          // Update the previously pressed buttons
          // This is used to prevent user from getting a strike for holding a
          // button
          prevPressedButtons = pressed;

          // Check if the user wants to restart
          if (getKey() == '#') {
            turnOffAllOutputs();
            currState = WELCOME;
            break;
          }
        }

        // Turn off outputs
        turnOffAllOutputs();

        // Check if the current state is still playing, the user won
        if (currState == PLAYING) {
          currState = WINNER;
        }
        break;
      }
      case LOSER: {
        // Tell the user that they lost :(
        displayCenteredTexts("You lost :(", "Rock on and", "try again!", "Press #");

        // Wait for a button press to restart the game
        while (getKey() != '#')
          ;

        // Move back to the welcome screen
        currState = WELCOME;
        break;
      }
      case WINNER: {
        // Tell the user that they won :)
        displayCenteredTexts("You won!", "Radical!", "", "Press #");

        // Wait for a button press to restart the game
        while (getKey() != '#')
          ;

        currState = WELCOME;
        break;
      }
    }
  }
}

/**
 * @brief Returns the duration of the previous note. Will return note 0's duration if the current note is 0
 *
 * @param currentNote The current note index
 * @return uint32_t The duration of the previous note
 */
uint32_t getPrevNoteDuration(uint8_t currentNote) {
  if (currentNote == 0) {
    return durations[0];
  }
  return durations[currentNote - 1];
}

/**
 * @brief Initializes the user LEDs on the board (P1.0 and P4.7)
 *
 */
void initUserLeds() {
  // Set pins to be digital IO
  P1SEL &= ~BIT0;
  P4SEL &= ~BIT7;

  // Set pins to be outputs
  P1DIR |= BIT0;
  P4DIR |= BIT7;

  // Turn off LEDs
  P1OUT &= ~BIT0;
  P4OUT &= ~BIT7;
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
 * @brief Gets the currently pressed buttons
 *
 * @return uint8_t First 4 bits represent if the buttons pressed
 */
uint8_t getPressedButtons() {
  uint8_t pressed = 0;

  // Check if the buttons are pressed
  if (!(P7IN & BIT0)) {
    pressed |= BIT3;
  }
  if (!(P3IN & BIT6)) {
    pressed |= BIT2;
  }
  if (!(P2IN & BIT2)) {
    pressed |= BIT1;
  }
  if (!(P7IN & BIT4)) {
    pressed |= BIT0;
  }

  return pressed;
}

/**
 * @brief Initializes timer A
 *
 */
void initTimerA() {
  // Initialize XT1 and XT2
  P5SEL |= (BIT2 | BIT3);  // Select XT1
  P5SEL |= (BIT4 | BIT5);  // Select XT2

  // Configure Timer A2 to use SMCLK, divide by 1, up mode
  TA2CTL = (TASSEL__SMCLK | ID__1 | MC__UP);

  // Timer period in ticks
  // SMCLK is 1.048576 million ticks per second
  // Divide by 2000 to get 524.288 ticks per second
  // Error will be mitigated by leap counting
  TA2CCR0 = 1048576 / (1000 * 2);

  // Enable interrupts for Timer A2
  TA2CCTL0 |= CCIE;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR() {
  // Timer ticks at 524
  // Should be 524.288 ticks per count (0.5ms)
  // 524.288 / 524 = 1.0005496
  // Error is only 0.05%... not worth leap counting for

  // Increment the counter
  A2Count++;
}

/**
 * @brief Get count of Timer A2. MUST BE USED TO PREVENT READING ISSUES
 *
 * @return uint32_t The count of timer A2 (in ms)
 */
uint32_t getTimerA2Millis() {
  __disable_interrupt();
  uint32_t count = A2Count / 2;
  __enable_interrupt();
  return count;
}

/**
 * @brief Resets the count of Timer A2. MUST BE USED TO PREVENT READING ISSUES
 *
 */
void resetTimerA2Count() {
  __disable_interrupt();
  A2Count = 0;
  __enable_interrupt();
}

/**
 * @brief Initializes the buzzer
 *
 */
void initBuzzer() {
  // Initialize PWM output on P3.5, which corresponds to TB0.5
  P3SEL |= BIT5;  // Select peripheral output mode for P3.5
  P3DIR |= BIT5;

  // Configure Timer B0 to use SMCLK, divide by 1, up mode
  TB0CTL = (TBSSEL__SMCLK | ID__1 | MC__UP);
  TB0CTL &= ~TBIE;  // Explicitly disable timer interrupts for safety

  // Disable both capture/compare periods (buzzer shouldn't be on yet)
  TB0CCTL0 = 0;  // Period of PWM
  TB0CCTL5 = 0;  // Duty cycle of PWM
}

/**
 * @brief Sets the user LEDs to the given value (P1.0 and P4.7 for bit 0 and 1
 * respectively)
 *
 * @param leds The value to set the LEDs to
 */
void displayUserLeds(uint8_t leds) {
  if (leds & BIT0) {
    P1OUT |= BIT0;
  } else {
    P1OUT &= ~BIT0;
  }

  if (leds & BIT1) {
    P4OUT |= BIT7;
  } else {
    P4OUT &= ~BIT7;
  }
}

/**
 * @brief Resets the note timer and lights the LEDs on the board corresponding to the given frequency
 *
 */
void showNote(uint16_t freq) {
  resetTimerA2Count();
  setLeds(freqToNoteBitGroup(freq));
}

/**
 * @brief Turns all LEDs and the buzzer off
 *
 */
void turnOffAllOutputs() {
  BuzzerOff();
  displayUserLeds(0b00);
  setLeds(0b0000);
}

/**
 * @brief Sounds the buzzer with the given note and duration
 *
 * @param freq The frequency of the note to play (Hz)
 */
void playNote(uint16_t freq) {
  // Configure the PWM period (controls frequency)
  // SMCLK has 1048576 ticks per second
  // Divide by the frequency to get the period
  TB0CCR0 = 1048576 / freq;
  TB0CCTL0 &= ~CCIE;  // Disable timer interrupts

  // Configure CC register 5, which is connected to our PWM pin TB0.5
  TB0CCTL5 = OUTMOD_3;    // Set/reset mode for PWM
  TB0CCTL5 &= ~CCIE;      // Disable capture/compare interrupts
  TB0CCR5 = TB0CCR0 / 2;  // Configure a 50% duty cycle
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
 * @param string4 The fourth string to display
 */
void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t* string3,
                          uint8_t* string4) {
  Graphics_clearDisplay(&g_sContext);
  Graphics_drawStringCentered(&g_sContext, string1, AUTO_STRING_LENGTH, 48, 15,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, string2, AUTO_STRING_LENGTH, 48, 30,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, string3, AUTO_STRING_LENGTH, 48, 45,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, string4, AUTO_STRING_LENGTH, 48, 60,
                              TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
}

/**
 * @brief Gives the user a strike
 *
 * @return If the user has lost
 */
bool giveStrike() {
  // Increment the number of strikes
  strikes++;

  // Show the user negative feedback
  displayUserLeds(0b01);

  // Update the display
  displayStrikes();

  // If the user has 3 strikes, they lose
  if (strikes == 3) {
    resetTimerA2Count();
    playNote(LAST_STRIKE_NOTE1);
    while (getTimerA2Millis() < LAST_STRIKE_DURATION)
      ;
    playNote(LAST_STRIKE_NOTE2);
    while (getTimerA2Millis() < LAST_STRIKE_DURATION * 2)
      ;
    playNote(LAST_STRIKE_NOTE3);
    while (getTimerA2Millis() < LAST_STRIKE_DURATION * 3)
      ;
    currState = LOSER;
    return true;
  }

  // User doesn't have 3 strikes yet, no need to break out of the loop
  return false;
}

/**
 * @brief Takes away a strike from the user
 *
 */
void takeAwayStrike() {
  // If the user has a strike, take it away
  if (strikes > 0) {
    strikes--;
  }

  // Show the user positive feedback
  displayUserLeds(0b10);

  // Update the display
  displayStrikes();
}

/**
 * @brief Displays the number of strikes the user has
 *
 */
void displayStrikes() {
  switch (strikes) {
    case 0: {
      displayCenteredText("0 strikes");
      break;
    }
    case 1: {
      displayCenteredText("1 strike");
      break;
    }
    case 2: {
      displayCenteredText("2 strikes");
      break;
    }
    case 3: {
      displayCenteredText("3 strikes");
      break;
    }
    default: {
      displayCenteredText("ERROR");
      break;
    }
  }
}

/**
 * @brief Converts a given frequency into a bit group for the LEDs/buttons
 *
 * @param freq The frequency to convert
 * @return uint8_t The bit group for the frequency (0b0001, 0b0010, 0b0100, or
 * 0b1000)
 */
uint8_t freqToNoteBitGroup(uint16_t freq) {
  if (freq < NOTEG0_MIN) {
    return 0b0001;
  } else if (freq < NOTEG1_MIN) {
    return 0b0010;
  } else if (freq < NOTEG2_MIN) {
    return 0b0100;
  } else {
    return 0b1000;
  }
}
