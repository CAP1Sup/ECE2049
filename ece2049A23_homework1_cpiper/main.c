#include <msp430.h>
#include <stdlib.h>

#define ALPHA 0.85f
#define READINGS 400

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	// Variables
	int r[READINGS];
	int a[READINGS];
	int randNum;
	unsigned int index;

	// Populate the raw data
	for(index = 0; index < READINGS; index++) {
	    SELECT_RAND:
	    randNum = rand()-1500;
	    if (randNum <= -1500 || randNum >= 200) {
	        goto SELECT_RAND;
	    }
	    r[index] = randNum;
	}

	// The first value is just copied from raw data
	a[0] = r[0];

	// Loop through, computing and saving the exponential averages
	for (index = 1; index < READINGS; index++) {
	    a[index] = (1.0 - ALPHA) * r[index] + ALPHA * a[index - 1];
	}

	return 0;
}
