/*
 * pixel_mover.c
 *
 * Created: 3/14/2019 6:09:13 AM
 * Author : Tony
 */ 

#include <avr/io.h>
#include "timer.h"
#include "usart.h"

// 0.954 hz is lowest frequency possible with this function,
// based on settings in PWM_on()
// Passing in 0 as the frequency will stop the speaker from generating sound
void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR0B &= 0x08; } //stops timer/counter
		else { TCCR0B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR0A = 0xFFFF; }
		
		// prevents OCR0A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR0A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR0A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT0 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR0A = (1 << COM0A0) | (1 << WGM00);
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR0B = (1 << WGM02) | (1 << CS01) | (1 << CS00);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR0A = 0x00;
	TCCR0B = 0x00;
}

typedef struct task {
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct)(int);
} task;

// GLOBAL VARIABLES
unsigned char i;					// iterator used in controller_read()
unsigned char j;					// iterator used in controller_read()
unsigned char temp;					// temp variable to store bits coming in from SNES controller
unsigned char data;					// controller data we interpreted and assigned
unsigned char pause = 0x00;			// iterator used in controller_out()
unsigned char sent = 0xFF;			// used to make sure holding button doesn't do anything
									// "sent" is initialized to 0xFF to initially not match anything with "data"

// Input bits from SNES controller mapped out.
const unsigned char UP = 0x08;
const unsigned char DOWN = 0x04;
const unsigned char LEFT = 0x02;
const unsigned char RIGHT = 0x01;
const unsigned char SELECT = 0x20;
const unsigned char START = 0x10;
const unsigned char Y = 0x40;
const unsigned char B = 0x80;


// States for controller_read()
enum In_States {IN_START, IN_INIT, WAIT1, WAIT2, WAIT3, READ, FINAL} in_state;

// Reads in data from SNES controller.
// The SNES controller sends data serially based on its clock.
// We shift every bit over and store them in sets of 8 in "temp".
// Then, we take the data from "temp", invert them, and map to values in "data".
// We map out specific values in "data" to represent which buttons have been pressed.
int controller_read(int state) {
	
	switch(state) { // Transition actions
		
		case IN_START:
		state = IN_INIT;
		break;
		
		case IN_INIT:
		PORTA = 0x04;	// set latch = 1
		state = WAIT1;
		break;
		
		case WAIT1:
		if(i < 2) {		// wait for 2ms
			i++;
			} else if(i >=2 ) {
			PORTA = 0x00;	// set latch = 0
			i = 0;
			state = READ;
		}
		break;
		
		case WAIT2:
		if(i < 2) {		// wait for 2ms
			i++;
			} else if(i >= 2) {
			i = 0;
			temp = temp << 1;						// shift bits in "temp" over by 1 to accept next bit
			temp = temp + ((PINA & 0x08) >> 3);		// accepts next bit coming in from A3. This is the main data we receive from the snes controller.
			state = WAIT3;
		}
		break;
		
		case WAIT3:
		if(i < 4) {		// wait for 4ms
			i++;
			} else if(i >= 4 && j >= 7) {	// after 4ms and all bits have been read to determine correct input
			state = FINAL;
			} else if(i >= 4 && j < 7) {	// if not all the bits have been read to determine correct input
			PORTA = 0x00;				// reset clock to 0
			PORTA = 0x02;				// set clock to 1
			i = 0;
			state = WAIT2;
			j++;						// increment number of bits that have been read
		}
		break;
		
		case READ:
		PORTA = 0x02;	// set clock = 1
		j++;			// keeps track of how many bits have read
		state = WAIT2;
		break;
		
		case FINAL:
		state = IN_INIT;
		break;
	}
	
	switch(state) { // State actions
		
		case IN_INIT:		// resets all counters, ports, and temporary variables
		i = 0;
		j = 0;
		PORTA = 0x00;
		temp = 0;
		break;
		
		case WAIT1:
		break;
		
		case WAIT2:
		break;
		
		case WAIT3:
		break;
		
		case READ:
		// stores the bit coming in from A3 into "temp" and shifts it to the least significant bit
		temp = (PINA & 0x08) >> 3;
		break;
		
		case FINAL:
		// default value of "data" is 0 because nothing is being pressed
		data = 0x00;
		
		// This is to flip the bits in "temp" in order to map to the controller data mapped out above.
		temp = ~temp;
		
		// sets "data" to values 0x01 - 0x08 based on value of "temp"
		if(temp == UP) {
			data = 0x01;
			} else if(temp == DOWN) {
			data = 0x02;
			} else if(temp == LEFT) {
			data = 0x03;
			} else if(temp == RIGHT) {
			data = 0x04;
			} else if(temp == SELECT) {
			data = 0x05;
			} else if(temp == START) {
			data = 0x06;
			} else if(temp == Y) {
			data = 0x07;
			} else if(temp == B) {
			data = 0x08;
		}
		break;
		
		default:
		break;
	}
	
	return state;
}


// States for controller_out()
enum Out_States {OUT_START, OUTPUT} out_state;

// Sends data received from controller.
// The main function here is to send the data received from the SNES controller through the USART to the ArduinoUno.
// This also sends data to PORTC's bottom 4 bits that lights up LEDs.
int controller_out(int state) {
	
	switch(state) { // Transition actions
		
		case OUT_START:
		state = OUTPUT;
		break;
		
		case OUTPUT:
		pause = 0;
		PORTC = data;
		
		// This is for generating buzzer sounds when the d-pad is pressed.
		if(data == 0x01 || data == 0x02 || data == 0x03 || data == 0x04) {
			set_PWM(50);
		} else {
			set_PWM(0);
		}
		
		// Sends controller input.
		// It will not send again if the button is pressed down.
		if(USART_IsSendReady(0) && data != sent) {
			USART_Send(data, 0);
			sent = data;
		}
		break;
	}
	
	switch(state) { // State actions
		
		case OUT_START:
		pause = 0;
		break;
		
		case OUTPUT:
		break;
		
		default:
		break;
	}
	
	return state;
};

unsigned char task_counter = 0x00;
unsigned char task_sent = 0xFF;
unsigned char game_temp;
unsigned char game_data;
unsigned char level_counter = 0x00;

enum Game_States{GAME_START, DISPLAY1, DISPLAY2, DISPLAY3, PLAY1, PLAY2, PLAY3, GAME_OVER} game_state;
int game_manager(int state) {
	
	switch(state) { // Transition actions
		
		case GAME_START:
			if(data == 0x06) {				// When player presses start, transition to display first level
				state = DISPLAY1;
			}
		break;
		
		case DISPLAY1:
			if(data == 0x05) {
				state = GAME_START;
			}
			if(level_counter < 200) {
				level_counter++;
			} else if(level_counter >= 200) {
				level_counter = 0;
				state = PLAY1;
			}
		break;
		
		case DISPLAY2:
			if(data == 0x05) {
				state = GAME_START;
			}
			if(level_counter < 200) {
				level_counter++;
			} else if(level_counter >= 200) {
				level_counter = 0;
				state = PLAY2;
			}	
		break;
		
		case DISPLAY3:
			if(data == 0x05) {
				state = GAME_START;
			}
			if(level_counter < 200) {
				level_counter++;
			} else if(level_counter >= 200) {
				level_counter = 0;
				state = PLAY3;
			}
		break;
		
		case PLAY1:
			if(data == 0x05) {				// at any time when the player presses select, restart to game start
				state = GAME_START;
			}
			if(game_temp == 0xA2) {
				state = DISPLAY2;
			}
		break;
		
		case PLAY2:
			if(data == 0x05) {				// at any time when the player presses select, restart to game start
				state = GAME_START;
			}
			if(game_temp == 0xA3) {
				state = DISPLAY3;
			}
		break;
		
		case PLAY3:
			if(data == 0x05) {				// at any time when the player presses select, restart to game start
				state = GAME_START;
			}
			if(game_temp == 0xA4) {
				state = GAME_OVER;
			}
		break;
		
		case GAME_OVER:
			if(data == 0x05) {
				state = GAME_START;
			}
		break;
	}
	
	switch(state) { // State actions
		
// For all state actions:
// Every half second, the ATmega1284 sends serial data to the ArduinoUno.
// The data sent corresponds to the state or task the ArduinoUno then handles.
// The ArduinoUno then uses the data received to decide what actions to perform.

// For PLAY state:
// Every half second, the ATmega1284 also opens to receive data from the ArduinoUno.
// Depending on what data is received, the PLAY state will change states.
// Otherwise, the only way to exit the PLAY state will be to press the select button on
// the SNES controller to reset the game. 
		
		case GAME_START:
			if(task_counter < 50) {
				task_counter++;
			} else if(task_counter >= 50) {
				task_counter = 0;
				if(USART_IsSendReady(0) && task_sent != 0x20) {
					USART_Send(0x20, 0);
					task_sent = 0x20;
				}
			}
		break;
		
		case DISPLAY1:
			if(task_counter < 50) {
				task_counter++;
			} else if(task_counter >= 50) {
				task_counter = 0;
				if(USART_IsSendReady(0) && task_sent != 0x21) {
					USART_Send(0x21, 0);
					task_sent = 0x21;
				}
			}		
		break;
		
		case DISPLAY2:
		if(task_counter < 50) {
			task_counter++;
			} else if(task_counter >= 50) {
			task_counter = 0;
			if(USART_IsSendReady(0) && task_sent != 0x22) {
				USART_Send(0x22, 0);
				task_sent = 0x22;
			}
		}
		break;
		
		case DISPLAY3:
			if(task_counter < 50) {
				task_counter++;
			} else if(task_counter >= 50) {
				task_counter = 0;
				if(USART_IsSendReady(0) && task_sent != 0x23) {
					USART_Send(0x23, 0);
					task_sent = 0x23;
				}
			}
		break;
		
		case PLAY1:
			if(task_counter < 50) {
				task_counter++;
			} else if(task_counter >= 50) {
				task_counter = 0;
				if(USART_IsSendReady(0) && task_sent !=  0x24) {
					USART_Send(0x24, 0);
					task_sent = 0x24;
				}
				if(USART_HasReceived(0)) {
					game_temp = USART_Receive(0);
					game_data = game_temp & 0xF0;
				}
			}
		break;
		
		case PLAY2:
			if(task_counter < 50) {
				task_counter++;
			} else if(task_counter >= 50) {
				task_counter = 0;
				if(USART_IsSendReady(0) && task_sent !=  0x25) {
					USART_Send(0x25, 0);
					task_sent = 0x25; 
				}
				if(USART_HasReceived(0)) {
					game_temp = USART_Receive(0);
					game_data = game_temp & 0xF0;
				}
			}
		break;
		
		case PLAY3:
			if(task_counter < 50) {
				task_counter++;
			} else if(task_counter >= 50) {
				task_counter = 0;
				if(USART_IsSendReady(0) && task_sent !=  0x26) {
					USART_Send(0x26, 0);
					task_sent = 0x26;
				}
				if(USART_HasReceived(0)) {
				game_temp = USART_Receive(0);
				game_data = game_temp & 0xF0;
				}
			}
		break;
		
		case GAME_OVER:
			if(task_counter < 50) {
				task_counter++;
			} else if(task_counter >= 50) {
				task_counter = 0;
				if(USART_IsSendReady(0) && task_sent != 0x27) {
					USART_Send(0x27, 0);
					task_sent = 0x27;
				}
			}
		break;
		
		default:
		break;	
	}
	
	return state;	
};

int main(void)
{
    DDRA = 0x06; PORTA = 0x09; // Configure A3 as input. This is the data line for the SNES controller.
    DDRB = 0xFF; PORTB = 0x00; // COnfigure B's 8 pins as outputs. Initialize to 0's. This is for the PWM.
    DDRC = 0xFF; PORTC = 0x00; // Configure C's 8 pins as outputs. Initialize to 0's. This is for the LEDs.
    DDRD = 0xFF; PORTD = 0x00; // Configure D's 8 pins as outputs. Initialize to 0's. This is for Rx/Tx with ArduinoUno.
    
    unsigned short i = 0; // Scheduler and tasks iterator
    task tasks[3];
    const unsigned short numTasks = 3;
    
    tasks[i].state = IN_START;
    tasks[i].period = 1;
    tasks[i].elapsedTime = 1;
    tasks[i].TickFct = &controller_read;
    i++;
    tasks[i].state = OUT_START;
    tasks[i].period = 1;
    tasks[i].elapsedTime = 1;
    tasks[i].TickFct = &controller_out;
    i++;
    tasks[i].state = GAME_START;
    tasks[i].period = 1;
    tasks[i].elapsedTime = 1;
    tasks[i].TickFct = &game_manager;
    
    initUSART(0);
    TimerSet(1);
    TimerOn();
	PWM_on();
    
    while (1)
    {
	    // Scheduler code
	    for(i = 0; i < numTasks; i++) {
		    if(tasks[i].elapsedTime == tasks[i].period) {
			    tasks[i].state = tasks[i].TickFct(tasks[i].state);
			    tasks[i].elapsedTime = 0;
		    }
		    tasks[i].elapsedTime += 1;
	    }
    }
}

