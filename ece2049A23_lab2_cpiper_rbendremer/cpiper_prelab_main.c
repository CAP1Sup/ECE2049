/*
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
uint8_t notes[30] = {A, Bb, B,  C, Cs, D, Eb, E,  F, Fs, G,   Ab, A_H, A, Bb,
                     B, C,  Cs, D, Eb, E, F,  Fs, G, Ab, A_H, A,  Bb,  B, C};

// State
enum State { WELCOME, PLAYING, LOSER, WINNER };
enum State currState = WELCOME;

// Main
void main(void) {
  WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer. Always need to stop this!!
                             // You can then configure it properly, if desired

  // Init peripherals (timer for buzzer, buttons, etc.)

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
*/