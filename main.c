/*
 * ESmp.c
 *
 * Created: 25-04-2024 06:16:54 PM
 * Author : Param
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#define F_CPU 1600000UL
/*Direction registers*/
#define LCD_DIR			DDRA
#define LCD_CTRL_DIR	DDRC
#define LED_DIR			DDRB
#define US_DIR			DDRD

/*PORT registers*/
#define LCD_PORT		PORTA
#define LCD_CTRL_PORT	PORTC
#define US_PORT			PORTD
#define LED_PORT		PORTB

/*LCD Pins*/
#define RS				PC7
#define RW				PC1
#define EN				PC0

/*HC-SR04 pins*/
#define TRIG			PD4
#define ECHO			PD6

/*LED Pins*/
#define LED_Red			PB0
#define LED_Green		PB1
#define LED_Blue		PB2

/*Timer overflow*/
static volatile int		TIMER1_overflow		= 0;
static volatile int		TIMER1_value		= 0;

/*Timer 1 ISR*/
ISR(TIMER1_OVF_vect) {
	TIMER1_overflow++;
}

void delay_ms(int delay) {
	TCNT0 = 0x00;
	TCCR0 = 0x04; /*256 prescaler*/
	for (int cycle = delay; cycle < 20; cycle++) {
		while (!(TIFR & (1 << TOV0)));
		TIFR |= (1 << TOV0);
	}
	TCNT0 = 0x00;
}

void delay_us(int delay) {
	TCNT0 = 256 - (delay * 16);
	TCCR0 = 0x01;
	while (!(TIFR & (1 << TOV0)));
	TIFR |= (1 << TOV0);
	TCNT0 = 0x00;
}

/*LCD functions*/
void LCD_cmnd(unsigned char command) {
	LCD_PORT = command;
	LCD_CTRL_PORT &= ~	(1 << RS);
	LCD_CTRL_PORT &= ~	(1 << RW);
	LCD_CTRL_PORT |=	(1 << EN);
	delay_ms(20);
	LCD_CTRL_PORT &= ~	(1 << EN);
	return;
}

void LCD_char(unsigned char character) {
	LCD_PORT = character;
	LCD_CTRL_PORT |=	(1 << RS);
	LCD_CTRL_PORT &= ~	(1 << RW);
	LCD_CTRL_PORT |=	(1 << EN);
	delay_ms(20);
	LCD_CTRL_PORT &= ~	(1 << EN);
	return;
}

void LCD_string(char *str) {
	for (int i = 0; str[i] != 0; i++) {
		LCD_char(str[i]);
	}
}

void LCD_init(void) {
	LCD_CTRL_DIR =		0xFF;
	LCD_DIR		 =		0xFF;
	delay_ms(20);
	LCD_cmnd(0x38);				/*LCD => 8 bit mode*/
	delay_ms(1);
	LCD_cmnd(0x0C);				/*Display 1 Cursor 0*/
	delay_ms(1);
	LCD_cmnd(0x06);				/*Auto increment cursor*/
	delay_ms(1);
	LCD_cmnd(0x01);				/*Clear display*/
	delay_ms(1);
	LCD_cmnd(0x80);				/*Cursor at home position*/
	delay_ms(1);
	LCD_string("Distance");
	LCD_cmnd(0xC0);				/*Go to second line*/
}
void LED_init() {
	LED_DIR |= (1 << LED_Blue);
	LED_DIR |= (1 << LED_Green);
	LED_DIR |= (1 << LED_Red);
}

/*HC-SR04 functions*/
void US_init(void) {
	US_DIR		|=		(1 << TRIG);
}

void US_trig(void) {
	US_PORT		 |=		(1 << TRIG);
	delay_us(15);
	US_PORT		 &= ~	(1 << TRIG);
}

int main(void) {
	double		Time;
	int			Distance;
	char		str[10];
	LCD_init();	
	US_init();
	LED_init();
	sei();
	TIMSK = (1 << TOIE1);
    while (1) {
		LCD_cmnd(0xC0);
		US_trig();											/*Trigger TRIG pin of HC-SR04*/
		TCNT1 = 0x00;
		TCCR1A = 0x00;
		TCCR1B = 0x40;
		TIFR |= (1 << ICF1);
		TIFR |= (1 << TOV1);
		
		while ((TIFR & (1 << ICF1)) == 0);
		TCNT1 = 0;	/* Clear Timer counter */
		TCCR1B = 0x01;	/* Capture on falling edge, No prescaler */
		TIFR = 1<<ICF1;	/* Clear ICP flag (Input Capture flag) */
		TIFR = 1<<TOV1;	/* Clear Timer Overflow flag */
		TIMER1_overflow = 0;/* Clear Timer overflow count */
		
		while ((TIFR & (1 << ICF1)) == 0);
		Time = (double) (TIMER1_overflow * 65535) + ICR1;		/*Calculate distance*/
		Distance = (int) Time / 55;
		dtostrf(Distance, 3, 0, str);
		strcat(str, "cm ");
		LCD_string(str);
		if (Distance > 30 && Distance < 200) {
			LED_PORT	&=~	(1 << LED_Blue);
			LED_PORT	|=	(1 << LED_Green);
			LED_PORT	|=	(1 << LED_Red);
		} else if (Distance > 200) {
			LED_PORT	&=~	(1 << LED_Green);
			LED_PORT	|=	(1 << LED_Blue);
			LED_PORT	|=	(1 << LED_Red);
		} else {
			LED_PORT	&=~	(1 << LED_Red);
			LED_PORT	|=	(1 << LED_Green);
			LED_PORT	|=	(1 << LED_Blue);
		}
    }
}

