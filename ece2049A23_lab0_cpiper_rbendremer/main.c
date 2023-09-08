//***************************************************************************************
//  MSP430 Blink the LED Demo - Software Toggle P6.2
//
//  Description; Toggle red LED by xor'ing P6.2 inside of a software loop.
//  ACLK = n/a, MCLK = SMCLK = default DCO
//
//                MSP430F5529
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |             P6.2|-->LED
//
//***************************************************************************************

#include <msp430.h>

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer

    // Set digital IO control registers for Port 6 Pin 2
    P6SEL &= ~BIT2;          // Select P6.2 for digital IO
    P6DIR |= BIT2;                  // Set P6.2 to output direction
    __disable_interrupt();          // Not using interrupts so disable them

    while(1)   // forever loop
    {
        volatile unsigned int i;    // volatile to prevent optimization
                                    // by compiler

        P6OUT ^= BIT2;      // Toggle P6.2 using exclusive-OR
        //P6OUT &= ~BIT2;   // Test whether logic logic 0 lights LED
        //P6OUT |= BIT2;        // or logic logic 1 lights LED

        i = 50000;    // Simple SW Delay
        while (i != 0)    // could have used while (i)
           i--;
    }

    return 0;
}
