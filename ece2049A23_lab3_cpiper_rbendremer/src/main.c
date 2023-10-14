#include "main.h"

// Op settings
#define DISPLAY_TIME 3       // seconds
#define UPDATE_TIME 1        // seconds
#define TEMP_AVG_SAMPLES 30  // one per second
#define POT_AVG_SAMPLES 30

// Conversions
#define SEC_PER_MIN 60UL
#define SEC_PER_HOUR 3600UL
#define SEC_PER_DAY 86400UL

// Last button states
// Used to prevent multiple presses from being registered
bool lastLeftButtonState = false;
bool lastRightButtonState = false;

// Counter ticks every 1s
// Overflow every (1s * (2^32 - 1)) = 4,294,967,295 seconds = ~136.2 years
// My (Christian's) is 11/05, which is the 309th day of the year (in 2023)
// Therefore we need to initialize the counter to:
// 309 days * 24 hours * 60 minutes * 60 seconds = 26,697,600 seconds
uint32_t A2Count = 309 * SEC_PER_DAY;

// Days in each month (in 2023, non-leap year)
uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
char months[12][3] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
uint32_t editingSeconds = 0;
uint8_t editIndex = 0;

// Bits to C conversion
// Set in initADC()
float degC_per_bit = 0.0f;

float tempReadings[TEMP_AVG_SAMPLES];
uint8_t tempIndex = 0;
uint8_t tempCount = 0;
enum ADCState { TEMP, POT };
enum ADCState currADCState = TEMP;
uint16_t potReadings[POT_AVG_SAMPLES];
uint8_t potIndex = 0;
uint8_t potCount = 0;

// State
enum State { DATE, EDIT_DATE, TIME, EDIT_TIME, TEMP_C, TEMP_F };
enum State currState = DATE;
uint32_t lastUpdate = 0;
uint32_t lastStateUpdate = 0;

// Main
void main(void) {
  WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer. Always need to stop this!!
                             // You can then configure it properly, if desired

  // Enable global interrupts
  _BIS_SR(GIE);

  // Init peripherals (timer for buzzer, buttons, etc.)
  initLeds();
  initButtons();
  initTimerA();
  initADC();
  configDisplay();
  configKeypad();

  // Main loop
  while (1) {
    // Check if the left button is pressed (meaning we need to go into edit
    // mode)
    if (isLeftButtonPressed()) {
      // Set the state to edit mode
      switch (currState) {
        case EDIT_DATE:
          editIndex++;
          if (editIndex > 1) {
            editIndex = 0;
            currState = EDIT_TIME;
          }
          break;
        case EDIT_TIME:
          editIndex++;
          if (editIndex > 2) {
            editIndex = 0;
            currState = EDIT_DATE;
          }
          break;
        case TIME:
          currState = EDIT_TIME;
          editIndex = 0;
          setupADCPot();
          editingSeconds = getSec();
          break;
        default:
          currState = EDIT_DATE;
          editIndex = 0;
          setupADCPot();
          editingSeconds = getSec();
          break;
      }
    } else if (isRightButtonPressed()) {
      // Set the state to edit mode
      switch (currState) {
        case EDIT_DATE:
          setTimerA2CountSec(editingSeconds);
          currState = DATE;
          setupADCContTemp();
          break;
        case EDIT_TIME:
          setTimerA2CountSec(editingSeconds);
          currState = TIME;
          setupADCContTemp();
          break;
        default:
          break;
      }
    }
    switch (currState) {
      case DATE:
        // Display the date from the A2 counter
        if (getSec() - lastUpdate >= UPDATE_TIME) {
          lastUpdate = getSec();
          displayDate(getSec() / SEC_PER_DAY);
        }

        // Change to time after 3 seconds passes
        if (getSec() - lastStateUpdate >= DISPLAY_TIME) {
          currState = TIME;
          lastStateUpdate = getSec();
        }
        break;
      case EDIT_DATE: {
        // Get the time from the A2 counter
        uint32_t seconds = editingSeconds;

        // Subtract the value being edited, then add the potentiometer reading
        if (editIndex == 0) {
          seconds -=
              monthToDays(daysToMonth(seconds / SEC_PER_DAY)) * SEC_PER_DAY;
          seconds +=
              monthToDays((uint8_t)(getPot() / 4096.0f * 12)) * SEC_PER_DAY;
        } else if (editIndex == 1) {
          uint8_t month = daysToMonth(seconds / SEC_PER_DAY);
          seconds -= monthToDays(month) * SEC_PER_DAY;
          seconds -= (seconds / SEC_PER_DAY) * SEC_PER_DAY;
          uint8_t daysInThisMonth =
              daysInMonth[daysToMonth(seconds / SEC_PER_DAY)];
          seconds +=
              ((uint32_t)((getPot() / 4096.0f) * (daysInMonth[month])) + 1) *
              SEC_PER_DAY;
          seconds += monthToDays(month) * SEC_PER_DAY;
        }

        // Compare the previously edited seconds to the current calculated
        // value for seconds
        if (seconds != editingSeconds) {
          // Update the editing seconds
          editingSeconds = seconds;

          // Display the time
          // Convert seconds to months and days
          uint8_t month = daysToMonth(editingSeconds / SEC_PER_DAY);
          uint16_t day = remainingDays(editingSeconds / SEC_PER_DAY);

          // Convert to strings
          char outputString[7];
          outputString[0] = months[month][0];
          outputString[1] = months[month][1];
          outputString[2] = months[month][2];
          outputString[3] = ' ';
          outputString[4] = day / 10 + '0';
          outputString[5] = day % 10 + '0';
          outputString[6] = '\0';

          Graphics_clearDisplay(&g_sContext);
          Graphics_drawStringCentered(&g_sContext, outputString,
                                      AUTO_STRING_LENGTH, 48, 15,
                                      TRANSPARENT_TEXT);
          uint8_t xPos = editIndex * 24 + 30;
          if (editIndex == 0) {
            Graphics_drawLineH(&g_sContext, xPos, xPos + 16, 20);
          } else if (editIndex == 1) {
            Graphics_drawLineH(&g_sContext, xPos, xPos + 10, 20);
          }
          Graphics_flushBuffer(&g_sContext);
        }
        break;
      }
      case TIME:
        // Display the time from the A2 counter
        // Get the time
        if (getSec() - lastUpdate >= UPDATE_TIME) {
          lastUpdate = getSec();
          displayTime(getSec());
        }

        // Change to temp in C after 3 seconds passes
        if (getSec() - lastStateUpdate >= DISPLAY_TIME) {
          currState = TEMP_C;
          lastStateUpdate = getSec();
        }
        break;
      case EDIT_TIME: {
        // Get the time from the A2 counter
        uint32_t seconds = editingSeconds;

        // Subtract the value being edited, then add the potentiometer reading
        if (editIndex == 0) {
          seconds -= ((seconds / SEC_PER_HOUR) % 24) * SEC_PER_HOUR;
          seconds += (uint32_t)(getPot() / 4096.0f * 24) * SEC_PER_HOUR;
        } else if (editIndex == 1) {
          seconds -= ((seconds / SEC_PER_MIN) % 60) * SEC_PER_MIN;
          seconds += (uint32_t)(getPot() / 4096.0f * 60) * SEC_PER_MIN;
        } else if (editIndex == 2) {
          seconds -= seconds % SEC_PER_MIN;
          seconds += (uint32_t)(getPot() / 4096.0f * 60);
        }

        // Compare the previously edited seconds to the current calculated
        // value for seconds
        if (seconds != editingSeconds) {
          // Update the editing seconds
          editingSeconds = seconds;

          // Display the time
          uint32_t timeInSeconds = editingSeconds;

          // Lop off the days
          timeInSeconds %= SEC_PER_DAY;

          // Convert to hours, minutes, seconds
          uint8_t hour = timeInSeconds / SEC_PER_HOUR;
          timeInSeconds %= SEC_PER_HOUR;
          uint8_t minute = timeInSeconds / SEC_PER_MIN;
          timeInSeconds %= SEC_PER_MIN;
          uint8_t second = timeInSeconds;

          // Convert to strings
          char outputString[9];
          outputString[0] = hour / 10 + '0';
          outputString[1] = hour % 10 + '0';
          outputString[2] = ':';
          outputString[3] = minute / 10 + '0';
          outputString[4] = minute % 10 + '0';
          outputString[5] = ':';
          outputString[6] = second / 10 + '0';
          outputString[7] = second % 10 + '0';
          outputString[8] = '\0';

          // Display the string
          Graphics_clearDisplay(&g_sContext);
          Graphics_drawStringCentered(&g_sContext, outputString,
                                      AUTO_STRING_LENGTH, 48, 15,
                                      TRANSPARENT_TEXT);
          uint8_t xPos = editIndex * 18 + 24;
          Graphics_drawLineH(&g_sContext, xPos, xPos + 10, 20);
          Graphics_flushBuffer(&g_sContext);
        }
        break;
      }
      case TEMP_C:
        // Display the average temperature in C
        if (getSec() - lastUpdate >= UPDATE_TIME) {
          lastUpdate = getSec();
          displayTempC(getTempCAvg());
        }

        // Change to temp in F after 3 seconds passes
        if (getSec() - lastStateUpdate >= DISPLAY_TIME) {
          currState = TEMP_F;
          lastStateUpdate = getSec();
        }
        break;
      case TEMP_F:
        // Display the average temperature in F
        if (getSec() - lastUpdate >= UPDATE_TIME) {
          lastUpdate = getSec();
          displayTempF(C_to_F(getTempCAvg()));
        }

        // Change to date after 3 seconds passes
        if (getSec() - lastStateUpdate >= DISPLAY_TIME) {
          currState = DATE;
          lastStateUpdate = getSec();
        }
        break;
    }
  }
}

/**
 * @brief Initializes the buttons on the board
 *
 */
void initButtons() {
  // Buttons are on: S1: P2.1, S2: P1.1

  // Set pins to be digital IO
  P2SEL &= ~BIT1;
  P1SEL &= ~BIT1;

  // Set pins to be inputs
  P2DIR &= ~BIT1;
  P1DIR &= ~BIT1;

  // Set internal resistors to pull-ups
  P2OUT |= BIT1;
  P1OUT |= BIT1;

  // Enable pull-up/down resistors
  P2REN |= BIT1;
  P1REN |= BIT1;
}

/**
 * @brief Checks if the left button is pressed
 *
 */
bool isLeftButtonPressed() {
  bool isPressed = !(P2IN & BIT1);
  if (isPressed && !lastLeftButtonState) {
    lastLeftButtonState = true;
    return true;
  } else if (!isPressed) {
    lastLeftButtonState = false;
  }
  return false;
}

/**
 * @brief Checks if the right button is pressed
 *
 */
bool isRightButtonPressed() {
  bool isPressed = !(P1IN & BIT1);
  if (isPressed && !lastRightButtonState) {
    lastRightButtonState = true;
    return true;
  } else if (!isPressed) {
    lastRightButtonState = false;
  }
  return false;
}

/**
 * @brief Initializes timer A
 *
 */
void initTimerA() {
  // Initialize XT1 and XT2
  P5SEL |= (BIT2 | BIT3);  // Select XT1
  P5SEL |= (BIT4 | BIT5);  // Select XT2

  // Configure Timer A2 to use ACLK, divide by 1, up mode
  TA2CTL = (TASSEL__ACLK | ID__1 | MC__UP);

  // Timer period in ticks
  // ACLK is 32768 ticks per second
  // So 32768 ticks per second / 1 = 32768 ticks per second
  // Subtract 1 because the timer counts from 0
  // No error, so no need for leap counting
  TA2CCR0 = 32768 - 1;

  // Enable interrupts for Timer A2
  TA2CCTL0 |= CCIE;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR() {
  // Counting is perfect, no need for leap counting

  // Increment the counter
  A2Count++;

  // Tell ADC to update the temp
  if (currADCState == TEMP) {
    ADC12CTL0 &= ~ADC12SC;
    ADC12CTL0 |= ADC12SC;
  }
}

/**
 * @brief Initializes the ADC
 *
 */
void initADC() {
  // Calculate the degC_per_bit conversion
  degC_per_bit =
      ((float)(85.0f - 30.0f)) / ((float)(CALADC12_15V_85C - CALADC12_15V_30C));

  // Configure P8.0 as digital IO output and set it to 1
  // This supplied 3.3 volts across scroll wheel potentiometer
  // See lab board schematic
  P8SEL &= ~BIT0;
  P8DIR |= BIT0;
  P8OUT |= BIT0;

  // Setup potentiometer pin (P6.0 = A0)
  P6SEL |= BIT0;  // Set pin to be analog input

  // Configure ADC12
  REFCTL0 &= ~REFMSTR;  // Reset REFMSTR to hand over control of
                        // internal reference voltages to
                        // ADC12_A control registers

  ADC12CTL0 &= ~ADC12ENC;  // Disable conversions

  // Configure ADC12
  ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON;  // Internal ref = 1.5V
  ADC12CTL1 = ADC12SHP;                            // Enable sample timer
  ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC i/p ch A10 = temp sense
  ADC12MCTL1 = ADC12SREF_0 + ADC12INCH_0;   // ADC i/p ch A0 = potentiometer

  // Enable ADC12 interrupts on conversion complete
  ADC12IE = ADC12IE0 | ADC12IE1;

  __delay_cycles(100);    // delay to allow Ref to settle
  ADC12CTL0 |= ADC12ENC;  // Enable conversion
}

// Configure the ADC ISR
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR() {
  // Check which mode we're in
  if (currADCState == TEMP) {
    // Set the temperature reading
    tempReadings[tempIndex] =
        (float)((long)ADC12MEM0 - CALADC12_15V_30C) * degC_per_bit + 30.0;

    // Increment the index
    tempIndex++;

    // Check if we've reached the end of the array
    if (tempIndex >= TEMP_AVG_SAMPLES) {
      tempIndex = 0;
    }

    // Increment the count if we haven't reached the max
    if (tempCount < TEMP_AVG_SAMPLES) {
      tempCount++;
    }

  } else {  // POT
    // Set the potentiometer reading
    // Using inverse because moving up is more logical
    potReadings[potIndex] = 4095 - ADC12MEM1;

    // Increment the index
    potIndex++;

    // Check if we've reached the end of the array
    if (potIndex >= POT_AVG_SAMPLES) {
      potIndex = 0;
    }

    // Increment the count if we haven't reached the max
    if (potCount < POT_AVG_SAMPLES) {
      potCount++;
    }

    // Reset and set the start bit of the ADC for the next conversion
    ADC12CTL0 &= ~ADC12SC;
    ADC12CTL0 |= ADC12SC;
  }
}

/**
 * @brief Configures the ADC to sample the temperature sensor
 *
 */
void setupADCContTemp() {
  __disable_interrupt();
  ADC12CTL0 &= ~ADC12ENC;  // Disable conversions

  // Set the ADC to sample the temperature sensor when triggered
  ADC12CTL1 = ADC12SHP | ADC12CSTARTADD_0;  // Enable sample timer

  // Set the ADC state (used for the ISR)
  currADCState = TEMP;

  ADC12CTL0 |= ADC12ENC;  // Enable conversion
  __enable_interrupt();
}

/**
 * @brief Calculates the average temperature in C from the ADC readings
 *
 * @return float The average temperature in C
 */
float getTempCAvg() {
  // Calculate the average
  float averageTempC = 0.0f;
  uint8_t i;
  for (i = 0; i < tempCount; i++) {
    averageTempC += tempReadings[i];
  }
  averageTempC /= tempCount;

  return averageTempC;
}

/**
 * @brief Converts the given temperature in C to F
 *
 * @param tempC The temperature in C
 * @return float The temperature in F
 */
float C_to_F(float tempC) { return tempC * 9.0f / 5.0f + 32.0f; }

/**
 * @brief Configures the ADC to sample the potentiometer
 *
 */
void setupADCPot() {
  __disable_interrupt();
  ADC12CTL0 &= ~ADC12ENC;  // Disable conversions

  // Set the ADC to sample the potentiometer when triggered
  ADC12CTL1 = ADC12SHP | ADC12CSTARTADD_1;  // Enable sample timer

  // Set the ADC state (used for the ISR)
  currADCState = POT;

  ADC12CTL0 |= (ADC12ENC | ADC12SC);  // Enable conversion and start sampling
  __enable_interrupt();
}

/**
 * @brief Gets the avg of the potentiometer readings.
 * MUST BE USED TO PREVENT READING ISSUES
 *
 * @return uint16_t The potentiometer reading
 */
uint16_t getPot() {
  // Calculate the average
  uint32_t sum = 0;

  __disable_interrupt();
  uint8_t i;
  for (i = 0; i < potCount; i++) {
    sum += potReadings[i];
  }
  sum /= potCount;
  __enable_interrupt();

  return sum;
}

/**
 * @brief Get the current time in seconds. MUST BE USED TO PREVENT READING
 * ISSUES
 *
 * @return uint32_t The current time in seconds
 */
uint32_t getSec() {
  __disable_interrupt();
  uint32_t count = A2Count;
  __enable_interrupt();
  return count;
}

/**
 * @brief Sets the count of Timer A2. MUST BE USED TO PREVENT ISSUES
 *
 */
void setTimerA2Count(uint8_t month, uint8_t day, uint8_t hour, uint8_t minute,
                     uint8_t second) {
  // Do all of the conversions before disabling interrupts
  uint16_t days = monthToDays(month) + day;
  uint32_t seconds =
      days * SEC_PER_DAY + hour * SEC_PER_HOUR + minute * SEC_PER_MIN + second;
  setTimerA2CountSec(seconds);
}

/**
 * @brief Sets the count of Timer A2. MUST BE USED TO PREVENT ISSUES
 *
 */
void setTimerA2CountSec(uint32_t seconds) {
  __disable_interrupt();
  A2Count = seconds;
  __enable_interrupt();
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
void displayCenteredText(char* string) {
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
 * @brief Calculates the month from the given number of days
 *
 * @param days The number of days
 * @return uint8_t The month
 */
uint8_t daysToMonth(uint32_t days) {
  uint8_t month = 0;
  while (days > daysInMonth[month]) {
    days -= daysInMonth[month];
    month++;
  }
  return month;
}

/**
 * @brief Calculates the remaining days in the month from the given number of
 * days
 *
 * @param days The number of days
 * @return uint8_t The remaining days
 */
uint8_t remainingDays(uint32_t days) {
  uint8_t month = 0;
  while (days > daysInMonth[month]) {
    days -= daysInMonth[month];
    month++;
  }
  return days;
}

/**
 * @brief Calculates the number of days from the given month
 *
 * @param month The month
 * @return uint16_t The number of days
 */
uint16_t monthToDays(uint8_t month) {
  uint16_t days = 0;
  uint8_t i;
  for (i = 0; i < month; i++) {
    days += daysInMonth[i];
  }
  return days;
}

/**
 * @brief Displays the given date in the center of the screen
 *
 * @param dateInSeconds The date in seconds to display
 */
void displayDate(uint32_t days) {
  // Convert seconds to months and days
  uint8_t month = daysToMonth(days);
  uint16_t day = remainingDays(days);

  // Convert to strings
  char outputString[7];
  outputString[0] = months[month][0];
  outputString[1] = months[month][1];
  outputString[2] = months[month][2];
  outputString[3] = ' ';
  outputString[4] = day / 10 + '0';
  outputString[5] = day % 10 + '0';
  outputString[6] = '\0';

  // Display the string
  displayCenteredText(outputString);
}

/**
 * @brief Displays the given time in seconds in the center of the screen
 *
 * @param timeInSeconds The time in seconds to display
 */
void displayTime(uint32_t timeInSeconds) {
  // Lop off the days
  timeInSeconds %= SEC_PER_DAY;

  // Convert to hours, minutes, seconds
  uint8_t hour = timeInSeconds / SEC_PER_HOUR;
  timeInSeconds %= SEC_PER_HOUR;
  uint8_t minute = timeInSeconds / SEC_PER_MIN;
  timeInSeconds %= SEC_PER_MIN;
  uint8_t second = timeInSeconds;

  // Convert to strings
  char outputString[9];
  outputString[0] = hour / 10 + '0';
  outputString[1] = hour % 10 + '0';
  outputString[2] = ':';
  outputString[3] = minute / 10 + '0';
  outputString[4] = minute % 10 + '0';
  outputString[5] = ':';
  outputString[6] = second / 10 + '0';
  outputString[7] = second % 10 + '0';
  outputString[8] = '\0';

  // Display the string
  displayCenteredText(outputString);
}

/**
 * @brief Displays the given temperature in C in the center of the screen
 *
 * @param tempInC The temperature in C to display
 */
void displayTempC(float tempInC) {
  // Convert to string
  if (tempInC > 100) {
    char outputString[7];
    outputString[0] = tempInC / 100 + '0';
    outputString[1] = (uint16_t)(tempInC / 10) % 10 + '0';
    outputString[2] = (uint16_t)tempInC % 10 + '0';
    outputString[3] = '.';
    outputString[4] = ((uint16_t)(tempInC * 10) % 10) + '0';
    outputString[5] = 'C';
    outputString[6] = '\0';

    // Display the string
    displayCenteredText(outputString);
  } else {
    char outputString[6];
    outputString[0] = (uint16_t)(tempInC / 10) % 10 + '0';
    outputString[1] = (uint16_t)tempInC % 10 + '0';
    outputString[2] = '.';
    outputString[3] = ((uint16_t)(tempInC * 10) % 10) + '0';
    outputString[4] = 'C';
    outputString[5] = '\0';

    // Display the string
    displayCenteredText(outputString);
  }
}

/**
 * @brief Displays the given temperature in F in the center of the screen
 *
 * @param tempInF The temperature in F to display
 */
void displayTempF(float tempInF) {
  // Convert to string
  if (tempInF > 100) {
    char outputString[7];
    outputString[0] = tempInF / 100 + '0';
    outputString[1] = (uint16_t)(tempInF / 10) % 10 + '0';
    outputString[2] = (uint16_t)tempInF % 10 + '0';
    outputString[3] = '.';
    outputString[4] = ((uint16_t)(tempInF * 10) % 10) + '0';
    outputString[5] = 'F';
    outputString[6] = '\0';

    // Display the string
    displayCenteredText(outputString);
  } else {
    char outputString[6];
    outputString[0] = (uint16_t)(tempInF / 10) % 10 + '0';
    outputString[1] = (uint16_t)tempInF % 10 + '0';
    outputString[2] = '.';
    outputString[3] = ((uint16_t)(tempInF * 10) % 10) + '0';
    outputString[4] = 'F';
    outputString[5] = '\0';

    // Display the string
    displayCenteredText(outputString);
  }
}
