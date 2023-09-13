/*
#include <msp430.h>
#include <peripherals.h>
#include <stdint.h>
#include <stdlib.h>

// Variables
uint8_t numList[50];  // Would prefer uint2_t for nums
uint8_t seqLen = 0;

// Current state
enum State { WELCOME, PLAYBACK, INPUT, WINNER };
enum State currState = WELCOME;

// main.c
int cpiper_prelab_main(void) {
  WDTCTL = WDTPW | WDTHOLD;  // stop watchdog timer

  // Init peripherals

  // Main loop
  while (1) {
    switch (currState) {
      case WELCOME: {
        // Display SIMON on screen

        // Do a count down

        // Move to the playback state
        currState = PLAYBACK;
        break;
      }
      case PLAYBACK: {
        // Generate a random number to add to the sequence

        // Increment the sequence length

        // Display the numbers one by one

        // Switch to the input state
        currState = INPUT;
        break;
      }
      case INPUT: {
        // Watch for input

        // Loop through the sequence

        // Check if the game needs restarted

        // Get the pressed buttons and check them against the sequence

        // Check if the user took too long

        // Increment the button checks

        // If the user was able to repeat the pattern successfully, move back to
        // displaying the numbers
        currState = PLAYBACK;
      }
      case WINNER: {
        // Tell the user that they won :)
        currState = WELCOME;
        break;
      }
    }
  }

  return 0;
}
  */
