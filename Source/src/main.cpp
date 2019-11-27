#include <Arduino.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

//#define OutPoPlusu			// Управление ШИМ через P-канальный полевой транзистор, т.е. по плюсу
#define OutPoMinus				// Управление ШИМ через N-канальный полевой транзистор, т.е. по минусу

#define CHANEL_1
//#define CHANEL_2

#define LED_PWM_1       OCR0A						// Первый канал PORTB.0
#define LED_PWM_2       OCR0B						// Второй канал PORTB.1

#define ALL_LED_ON		(PORTB |= _BV(PB0));\
				(PORTB |= _BV(PB1));
#define ALL_LED_OFF		(PORTB &= ~_BV(PB0));\
				(PORTB &= ~_BV(PB1));

#define ALL_LED_XOR		(PORTB ^= _BV(PB0));\
				(PORTB ^= _BV(PB1));

#define LED_XOR(x)      	(PORTB ^=  _BV(x))			// Переключение состояния первого или второго канала (нужно для режима программирования параметров)
#define LED_ON(x)		(PORTB |= _BV(x))
#define LED_OFF(x)		(PORTB &= ~_BV(x))

#define BLOK_RELE_OFF		(PORTB &= ~_BV(PB2))		// Включение выхода на блокировку внешнего питания 
#define BLOK_RELE_ON		(PORTB |=  _BV(PB2))		// Отключение выхода на блокировку внешнего питания
#define RELE			!(PINB & _BV(PB3))
#define KEY1            	!(PINB & _BV(PB4))			// Кнопка для входа в режим программирования параметров ПЕРВОГО канала
#define KEY2            	!(PINB & _BV(PB5))			// Кнопка для входа в режим программирования параметров ВТОРОГО канала
#define KEY(x)			!(PINB & _BV(1 << x))

#define stupen_disable		0
#define stupen_enable		1
#define speed_PWM_enable	2

const uint16_t  FreqMulti = 586;

bool 		flagStart 	= false;
uint16_t 	timerButton;			// задержка (антидребезк)

#include "DHL_1ch.h"
#include "DHL_2ch.h"
