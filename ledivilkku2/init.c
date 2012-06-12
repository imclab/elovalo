/*
 * init.c
 *
 *  Created on: 24.5.2012
 *      Author: Icchan
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

void initPorts(){

	/**
	* Port B initialization
	* PORTB Output all
	* Set to zero
	*/

	//PORTB=0x00;
	//DDRB=0xff;

	/** Port C initialization*/

	//PORTC=0x00;
	//DDRC=0x0ff;


	// USART0 Baud rate: 115200

/*
	__asm__ ("cli");
	__asm__ ("sei");
*/
}

void initSPI(){
	//DDRB = 0xFF;
	/* Set MOSI, !SS and SCK output, all others input */
	DDRB = (1<<PB3)|(1<<PB5)|(1<<PB2);

	SPCR = (0<<SPIE) | //We want interrupts
	(1<<SPE) | //We do want the SPI enabled
	(1<<DORD) | //We want the data to be shifted out LSB
	(1<<MSTR) | //We want the atmega to be a master
	(0<<CPOL) | //We want the leading edge to be rising
	(0<<CPHA) | //We want the leading edge to be sample
	(0<<SPR1) | (0<<SPR0) ; // sets the clock speed

	SPSR = (0<<SPIF) | // SPI interrupt flag
	(0<<WCOL) | //Write collision flag
	(1<<SPI2X) ; //Doubles the speed of the SPI clock

}

void initUSART(){

	uint16_t ubrr = 103; //(F_CPU/(16UL*BAUD_RATE))-1;

	// disable all interrupts before configuration
	cli();

	// USART0 Baud Rate Register
	// set clock divider
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;

    // Set frame format to 8 data bits, no parity, 1 stop bit
    UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
    //enable reception and RC complete interrupt
    UCSR0B |= (1<<RXEN0)|(1<<RXCIE0)|(1<<TXEN0);

}



//void Usartpuskuri_add)(struct usartpuskuri* buffer, struct usartpuskuri* newnode){
//buffer->
//}
