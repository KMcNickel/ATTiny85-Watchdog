/*
 * ATTiny85 Watchdog.c
 *
 * Created: 4/5/2018 14:52:32
 * Author : Kyle
 * 
 * Pinout:
 *	PB0 is the input from the device we are monitoring
 *	PB1 is the reset line to the device we are monitoring
 *	PB2 is a power cutoff output in case we have anything that should go to a known state in the event of failure of the device we are monitoring
 *	PB3 - 5 are delay time-setting pins
 *
 * Setting the delay:
 *	PB3 - 5 are set up like 3 bits. PB3 is the LSB (value 1), PB5 is the MSB (value 4)
 *	Value	Time
 *	1		500 milliseconds
 *	2		3 seconds
 *	3		5 seconds
 *	4		10 seconds
 *	5		15 seconds
 *	6		30 seconds
 *	7		60 seconds
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 1000000UL																//Set the clock to 1MHz

#define microsPerOverflow	18 * 256												//1 MHz = 1 cycle per millisecond * prescaler * buffer size
#define millisIncrement		microsPerOverflow / 1000								//How many whole millis we need to increment when we get an overflow
#define fractIncrement		(microsPerOverflow % 1000) >> 2							//1 byte representation of how inaccurate this system is so we can compensate (in microseconds)
#define fractMax			1000 >> 2												//1 byte representation of how many microseconds are in a millisecond (kinda)

unsigned long watchdogTime;															//The last time the device sent a rising edge
unsigned long resetTime;															//When the reset output was taken LOW

unsigned long millis();

unsigned long t0Millis;															//Updated when timer0 overflows
unsigned long timeFraction;															//Tracks the inaccuracy of the above number so we can fix it every ~44 ms

int delayLength;																	//How often the device has to send a rising edge

char watchdogStatus;
/*Bit 0 is the last input state (for edge detection)
 *Bit 1 is the status of the watched device (set to 0 if no response within time limit)
*/
int main(void)
{
	//B0: Watchdog Input
	//B1: MCU Reset Pin
	//B2: Power Cutoff
	//B3: Delay 1 (1) LSB
	//B4: Delay 2 (2)
	//B5: Delay 3 (4) MSB
	DDRB = 0b00000110;

	//Setting up registers to use Timer0 like the arduino millis() function
	GTCCR |= 1 << 7;																	//Pause the prescaler so we can set everything up
	TCCR0A = 0b00000000;																//No output compare, WGM normal
	TCCR0B = 0b00000010;																//No force output compare, WGM normal, prescaler 8
	TIMSK =	 0b00000010;																//No output compare interrupt, Enable overflow interrupt
	sei();																				//Enable interrupts globally
	GTCCR &= ~(1 >> 7);																	//Enable the prescaler

	PORTB = ~_BV(1) & ~_BV(2) & PORTB;													//Set PB1 and 2 low while we start up

	char delayVal = 0;																	//Get the values of the three delay setting pins and set up the delay length.
	if ((PINB & _BV(3)) >> 3) delayVal += 1;
	if ((PINB & _BV(4)) >> 4) delayVal += 2;
	if ((PINB & _BV(5)) >> 5) delayVal += 4;
	switch (delayVal) {
		case 0: delayLength = 500; break;
		case 1: delayLength = 1000; break;
		case 2: delayLength = 3000; break;
		case 3: delayLength = 5000; break;
		case 4: delayLength = 10000; break;
		case 5: delayLength = 15000; break;
		case 6: delayLength = 30000; break;
		case 7: delayLength = 60000; break;
		default: while(1) {};																	//In the off chance we get a different value, stop here (failsafe)
	}

	while (PINB & 1) {};																	//Since everything is off, the input should NOT be HIGH (failsafe)
	PORTB = (_BV(1) & _BV(2)) | PORTB;														//Turn everything on, we are done setting up

    while (1) 
    {
		unsigned long currentTime = millis();
		if ((PINB & 1) && !(watchdogStatus & 1)) {											//If we have a rising edge
			watchdogTime = currentTime;														//Update the time we last received the rising edge
			if (!((watchdogStatus >> 1) & 1)) {														//If the system was shutdown due to a prior failure
				PORTB |= _BV(1);																	//Turn the power cutoff back on
				watchdogStatus |= 2;																//and update the status
			}
		}
		if (((watchdogStatus >> 1) & 1) && (currentTime - watchdogTime >= delayLength)) {	//If we WERE doing fine, but we have exceeded our delay time
			PORTB = ~_BV(1) & ~_BV(2) & PORTB;													//Take both outputs low
			resetTime = currentTime;															//Make a note of the time so we can bring the reset back on
			watchdogStatus &= 253;																//and update the status
		}
		if (((watchdogStatus >> 1) & 1) && (currentTime - resetTime >= 100)) PORTB |= _BV(2);	//If we are not doing fine and the reset pin has been LOW for at least 0.1s, take it HIGH
		if (PINB & 1) {																			//Update the input state because edge detection
			watchdogStatus |= 1;
		} else watchdogStatus &= 254;
	}
}

ISR(TIMER0_OVF_vect) {
	t0Millis += millisIncrement;
	timeFraction += fractIncrement;
	if (timeFraction >= fractMax) {
		timeFraction -= fractMax;
		t0Millis += 1;
	}
}

unsigned long millis() {
	unsigned long m;
	char oldSREG = SREG;
	cli();
	m = t0Millis;
	SREG = oldSREG;
	return m;
}