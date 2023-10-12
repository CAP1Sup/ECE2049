// #include <main.h>
//
// #define DISPLAY_TIME 3  // seconds
//
//// Counter ticks every 1s
//// Overflow every (1s * (2^32 - 1)) = 4,294,967,295 seconds = ~136.2 years
//// My (Christian's) is 11/05, which is the 309th day of the year (in 2023)
//// Therefore we need to initialize the counter to:
//// 309 days * 24 hours * 60 minutes * 60 seconds = 26,697,600 seconds
// uint32_t A2Count = 26697600;
//
//// Days in each month (in 2023, non-leap year)
// uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//
//// State
// enum State { DATE, TIME, TEMP_C, TEMP_F };
// enum State currState = DATE;
// uint32_t lastUpdate = 0;
//
//// Main
// void main(void) {
//   WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer. Always need to stop
//   this!!
//                              // You can then configure it properly, if
//                              desired
//
//   // Enable global interrupts
//   _BIS_SR(GIE);
//
//   // Init peripherals (timer for buzzer, buttons, etc.)
//   initLeds();
//   initUserLeds();
//   initButtons();
//   initTimerA();
//   initADC();
//   configDisplay();
//   configKeypad();
//
//   // Main loop
//   while (1) {
//     switch (currState) {
//       case DATE:
//         // Display the date from the A2 counter
//         // Change to time after 3 seconds passes (according to the A2
//         // counter)
//         break;
//       case TIME:
//         // Display the time from the A2 counter
//         // Change to temp in C after 3 seconds passes (according to the A2
//         // counter)
//         break;
//       case TEMP_C:
//         // Query the ADC for the temperature in C, then average it
//         // Display the average temperature in C
//         // Change to temp in F after 3 seconds passes (according to the A2
//         // counter)
//         break;
//       case TEMP_F:
//         // Convert the average temperature in C to F
//         // Display the average temperature in F
//         // Change to date after 3 seconds passes (according to the A2
//         // counter)
//         break;
//     }
//   }
// }
//
///**
// * @brief Initializes the buttons on the board
// *
// */
// void initButtons() {
//  // Buttons are on: S1: P2.1, S2: P1.1
//
//  // Set pins to be digital IO
//  P2SEL &= ~BIT1;
//  P1SEL &= ~BIT1;
//
//  // Set pins to be inputs
//  P2DIR &= ~BIT1;
//  P1DIR &= ~BIT1;
//
//  // Set internal resistors to pull-ups
//  P2OUT |= BIT1;
//  P1OUT |= BIT1;
//
//  // Enable pull-up/down resistors
//  P2REN |= BIT1;
//  P1REN |= BIT1;
//}
//
///**
// * @brief Initializes timer A
// *
// */
// void initTimerA() {
//  // Initialize XT1 and XT2
//  P5SEL |= (BIT2 | BIT3);  // Select XT1
//  P5SEL |= (BIT4 | BIT5);  // Select XT2
//
//  // Configure Timer A2 to use ACLK, divide by 1, up mode
//  TA2CTL = (TASSEL__ACLK | ID__1 | MC__UP);
//
//  // Timer period in ticks
//  // ACLK is 32768 ticks per second
//  // So 32768 ticks per second / 1 = 32768 ticks per second
//  // Subtract 1 because the timer counts from 0
//  // No error, so no need for leap counting
//  TA2CCR0 = 32768 - 1;
//
//  // Enable interrupts for Timer A2
//  TA2CCTL0 |= CCIE;
//}
//
// #pragma vector = TIMER2_A0_VECTOR
//__interrupt void TimerA2_ISR() {
//  // Counting is perfect, no need for leap counting
//
//  // Increment the counter
//  A2Count++;
//}
//
///**
// * @brief Get the current time in seconds. MUST BE USED TO PREVENT READING
// * ISSUES
// *
// * @return uint32_t The current time in seconds
// */
// uint32_t getSec() {
//  __disable_interrupt();
//  uint32_t count = A2Count;
//  __enable_interrupt();
//  return count;
//}
//
///**
// * @brief Resets the count of Timer A2. MUST BE USED TO PREVENT READING ISSUES
// *
// */
//// ! TODO: Finish this with month adjustments
// void setTimerA2Count(uint8_t month, uint8_t day, uint8_t hour, uint8_t
// minute,
//                      uint8_t second) {
//   __disable_interrupt();
//   A2Count = month * 2678400 + day * 86400 + hour * 3600 + minute * 60 +
//   second;
//   __enable_interrupt();
// }
//
///**
// * @brief Clears the screen
// */
// void clearDisplay() {
//  Graphics_clearDisplay(&g_sContext);
//  Graphics_flushBuffer(&g_sContext);
//}
//
///**
// * @brief Displays the given string in the center of the screen
// *
// * @param string The string to display
// */
// void displayCenteredText(uint8_t* string) {
//  Graphics_clearDisplay(&g_sContext);
//  Graphics_drawStringCentered(&g_sContext, string, AUTO_STRING_LENGTH, 48, 15,
//                              TRANSPARENT_TEXT);
//  Graphics_flushBuffer(&g_sContext);
//}
//
///**
// * @brief Displays the given strings in the center of the screen
// *
// * @param string1 The first string to display
// * @param string2 The second string to display
// * @param string3 The third string to display
// * @param string4 The fourth string to display
// */
// void displayCenteredTexts(uint8_t* string1, uint8_t* string2, uint8_t*
// string3,
//                          uint8_t* string4) {
//  Graphics_clearDisplay(&g_sContext);
//  Graphics_drawStringCentered(&g_sContext, string1, AUTO_STRING_LENGTH, 48,
//  15,
//                              TRANSPARENT_TEXT);
//  Graphics_drawStringCentered(&g_sContext, string2, AUTO_STRING_LENGTH, 48,
//  30,
//                              TRANSPARENT_TEXT);
//  Graphics_drawStringCentered(&g_sContext, string3, AUTO_STRING_LENGTH, 48,
//  45,
//                              TRANSPARENT_TEXT);
//  Graphics_drawStringCentered(&g_sContext, string4, AUTO_STRING_LENGTH, 48,
//  60,
//                              TRANSPARENT_TEXT);
//  Graphics_flushBuffer(&g_sContext);
//}
//