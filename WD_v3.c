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
 *	0		500 milliseconds
 *	1		1 second
 *	2		3 seconds
 *	3		5 seconds
 *	4		10 seconds
 *	5		15 seconds
 *	6		30 seconds
 *	7		60 seconds
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef _BV
#define _BV(c) (1<<c)
#endif

#define _RR(a, b) ((a &_BV(b)) >> b)

#define F_CPU 1000000UL														//Set the clock to 1MHz

uint16_t count;

uint16_t max;

char rstCount;
char rstMax = 49;

int main(void)
{
	//B0: Watchdog Input
	//B1: MCU Reset Pin
	//B2: Power Cutoff
	//B3: Delay 1 (1) LSB
	//B4: Delay 2 (2)
	//B5: Delay 3 (4) MSB
	DDRB = 0b00000110;

	PORTB &= (~_BV(1) & ~_BV(2));											//Set PB1 and 2 low while we start up (in case they arent for some reason)

	char delayVal = 0;														//Get the values of the three delay setting pins and set up the delay length.
	if (PINB & _BV(3)) delayVal += 1;
	if (PINB & _BV(4)) delayVal += 2;
	if (PINB & _BV(5)) delayVal += 4;
	switch (delayVal) {														//All of these values were determined to be the "best" combination to minimize interrupts
		case 0: max = 244; break;												//500 ms
		case 1: max = 488; break;												//1 second
		case 2: max = 1465; break;												//3 seconds
		case 3: max = 2441; break;												//5 seconds
		case 4: max = 4883; break;												//10 seconds
		case 5: max = 7324; break;												//15 seconds
		case 6: max = 14648; break;												//30 seconds
		case 7: max = 29297; break;												//60 seconds
		default: while(1) {};													//In the off chance we get a different value, stop here (failsafe)
	}

	GTCCR |= _BV(7);														//Pause the prescaler so we can set everything up
	TCCR0A = 0b00000000;													//No output compare, WGM normal
	TCCR0B = 0b00000010;													//No force output compare, WGM normal, prescaler 8
	TIMSK =	 0b00000010;													//No output compare interrupt, Enable overflow interrupt

	while (PINB & 1) {};													//Since everything is off, the input should NOT be HIGH (failsafe)
	PORTB = (_BV(1) & _BV(2)) | PORTB;										//Turn everything on
	GTCCR &= ~_BV(7);														//Start the prescaler
	sei();																	//Enable interrupts globally, setup is done

	while(1);
//At this point, interrupts are handling everything
}

ISR(TIMER0_OVF_vect) {														//When Timer0 overflows
	if(!(PORTB & 2)) {															//If we have started a reset
		rstCount++;																	//Increase the reset timer count
		if(rstCount >= rstMax) {														//If we have reached the required number of overflows, we are done resetting
			rstCount = 0;																//Reset the count
			PORTB |= _BV(1);															//Take the reset pin high
		}
	}
	if(PORTB & 4) {																//If power is on
		count++;																	//Increase the timer count
		if(count >= max) {															//If we have reached the required number of overflows, the monitored device has a problem
			count = rstCount = 0;														//Reset both counts
			PORTB = ~_BV(1) & ~_BV(2) & PORTB;											//Take both outputs low (has to be after the reset count)
		}
	}
}

ISR(INT0_vect) {															//If we have a state change on PB0
	if (PINB & 1){																//And it is now high (rising edge)
		count = 0;																	//Reset our count
		if (!(PORTB & 4)) {															//If the system was shutdown due to a prior failure
			PORTB |= _BV(2);															//Turn the power cutoff back on
		}
	}
}