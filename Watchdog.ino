//Version 0.1
unsigned long resetTime;
unsigned long watchdogTime;
bool lastWDState;

bool watchdogStatus;

int delayLength;
/*Delay lengths are:
 * 0 - 0.5 seconds
 * 1 - 1 second
 * 2 - 3 seconds
 * 3 - 5 seconds
 * 4 - 10 seconds
 * 5 - 15 seconds
 * 6 - 30 seconds
 * 7 - 1 Minute
 */


void setup() {
  //B0: Watchdog Input
  //B1: MCU Reset Pin
  //B2: Power Cutoff
  //B3: Delay 1 (1) LSB
  //B4: Delay 2 (2)
  //B5: Delay 3 (4) MSB

  DDRB = B00000110;

  PORTB = ~_BV(1) & ~_BV(2) & PORTB;

  uint8_t delayVal;
  if ((PINB & _BV(3)) >> 3) delayVal += 1;
  if ((PINB & _BV(4)) >> 4) delayVal += 2;
  if ((PINB & _BV(5)) >> 5) delayVal += 4;
  switch (delayVal) {
    case 0:
      delayLength = 500;
      break;
    case 1:
      delayLength = 1000;
      break;
    case 2:
      delayLength = 3000;
      break;
    case 3:
      delayLength = 5000;
      break;
    case 4:
      delayLength = 10000;
      break;
    case 5:
      delayLength = 15000;
      break;
    case 6:
      delayLength = 30000;
      break;
    case 7:
      delayLength = 60000;
      break;
    default:
      while(1) delay(1);
  }
  while (PINB & 1) delay(1);
  PORTB = _BV(1) & _BV(2) | PORTB;
}

void loop() {
  unsigned long currentTime = millis();
  if ((PINB & 1) && !lastWDState) {
    watchdogTime = currentTime;
    if (!watchdogStatus) {
      PORTB |= _BV(1);
      watchdogStatus = 1;
    }
  }
  if (watchdogStatus && (currentTime - watchdogTime >= delayLength)) {
    PORTB = ~_BV(1) & ~_BV(2) & PORTB;
    resetTime = currentTime;
    watchdogStatus = 0;
  }
  if (!watchdogStatus && (currentTime - resetTime >= 100)) PORTB |= _BV(2);
  lastWDState = PINB & 1;
}
