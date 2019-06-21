#include <Arduino.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

//#define OutPoPlusu			// Управление ШИМ через P-канальный полевой транзистор, т.е. по плюсу
#define OutPoMinus				// Управление ШИМ через N-канальный полевой транзистор, т.е. по минусу

#define reset_enable
//#define reset_disable


//#define STUPEN          4       // Количество ступеней работы (3 - работа без 1 ступени, 4 - работают все 4 ступени)
//#define pcsCHANEL       1       // Количество каналов ШИМ (1 - работа двух каналов синхронно, 2 - работа двух каналов раздельно)



#define LED_PWM_1       OCR0A						// Первый канал PORTB.0
#define LED_PWM_2       OCR0B						// Второй канал PORTB.1

#define ALL_LED_ON		(PORTB |= _BV(PB0));\
						(PORTB |= _BV(PB1));
#define ALL_LED_OFF		(PORTB &= ~_BV(PB0));\
						(PORTB &= ~_BV(PB1));

#define ALL_LED_XOR		(PORTB ^= _BV(PB0));\
						(PORTB ^= _BV(PB1));

#define LED_XOR(x)      (PORTB ^=  _BV(x))			// Переключение состояния первого или второго канала (нужно для режима программирования параметров)
#define LED_ON(x)		(PORTB |= _BV(x))
#define LED_OFF(x)		(PORTB &= ~_BV(x))

#define BLOK_RELE_OFF	(PORTB &= ~_BV(PB2))		// Включение выхода на блокировку внешнего питания 
#define BLOK_RELE_ON	(PORTB |=  _BV(PB2))		// Отключение выхода на блокировку внешнего питания
#define RELE			!(PINB & _BV(PB3))
#define KEY1            !(PINB & _BV(PB4))			// Кнопка для входа в режим программирования параметров ПЕРВОГО канала
#define KEY2            !(PINB & _BV(PB5))			// Кнопка для входа в режим программирования параметров ВТОРОГО канала
#define KEY(x)			!(PINB & _BV(x+4))


#define stupen_disable		0
#define stupen_enable		1
#define speed_PWM_enable	2

#define FreqMulti			586

bool 		flagStart 	= false;

#ifdef reset_enable
	EEMEM uint16_t eeTime[4] = {0*FreqMulti, 10*FreqMulti, 5*FreqMulti, 15*FreqMulti};
	uint8_t		brightness = 255;
	uint8_t 	ledout;
	uint8_t 	stupen_PWM;			// Номер ступени [номер канала]
	uint8_t 	speed_PWM;			// Скорость изменения ШИМ [номер канала]
	uint16_t 	delay_PWM;			// Время работы ступени [номер канала]
#endif

#ifdef reset_disable
	EEMEM uint16_t eeTime[2][4] = {{0*FreqMulti,10*FreqMulti,5*FreqMulti,15*FreqMulti},{0*FreqMulti,5*FreqMulti,10*FreqMulti,15*FreqMulti}};   // Хранится время задержки включения, время плавного включения, время работы и плавного выключения каналов
	uint8_t		brightness[2] = {255, 255};
	uint8_t 	ledout[2];
	uint8_t 	stupen_PWM[2];			// Номер ступени [номер канала]
	uint8_t 	speed_PWM[2];			// Скорость изменения ШИМ [номер канала]
	uint16_t 	delay_PWM[2];			// Время работы ступени [номер канала]
#endif

uint16_t 	timerButton;			// задержка (антидребезк)


#ifdef reset_enable
	void miganie_svetikom (uint8_t x) {
		for (uint8_t z=0; z<=x; z++) {
			ALL_LED_XOR;
			_delay_ms(300);
		}
	}

	void key_button (void) {

		uint8_t timeProg;
		uint8_t timeProgEE;
		uint8_t nomer = 0;
		
		do {
			miganie_svetikom(5);
			
			while (KEY1);
			timeProg = 0;
			timeProgEE = 0;

			while (timeProg != 20) {
				_delay_ms(100);
				timeProg++;
				if (KEY1) {
					_delay_ms(100);
					if (KEY1) {
						ALL_LED_ON;
						timeProg = 0;
						while (KEY1);
						ALL_LED_OFF;
						if (++timeProgEE == 99) {break;}}}}
			eeprom_update_word(&eeTime[nomer],uint16_t(timeProgEE*FreqMulti));
			nomer++;
		} while (nomer != 4);
		miganie_svetikom(11);
	}

#endif

#ifdef reset_disable
	void miganie_svetikom (uint8_t x, uint8_t ch) {
		for (uint8_t z=0; z<=x; z++) {
			LED_XOR(ch);
			_delay_ms(300);
		}
	}

	void key_button (uint8_t ch) {											// функция изменения и записи в ЕЕПРОМ временных задержек (ch - указатель на то какая кнопка была нажата)

		uint8_t timeProg;
		uint8_t timeProgEE;
		uint8_t nomer;

		nomer = 0;

		do {
			miganie_svetikom(5,ch);										// моргаем 3 раза соответствующим каналом 
							
			while (KEY(ch));											// ждем пока не будет отпущена кнопка соответствующего канала (по маске 100000 >> 10 == 01 (KEY1) или 10 (KEY2)
			timeProg = timeProgEE = 0;

			while (timeProg != 20) {
				_delay_ms(100);
				timeProg++;
				if (KEY(ch)) {
					_delay_ms(100);
					if ((KEY(ch))) {
						LED_ON(ch);
						timeProg = 0;
						while (KEY(ch));
						LED_OFF(ch);
						if (++timeProgEE == 200) {break;}}}}
			eeprom_update_word(&eeTime[ch][nomer],timeProgEE*FreqMulti);
			nomer++;
		} while (nomer != 4);
		miganie_svetikom(11,ch);
	}

#endif

ISR (TIM0_OVF_vect) {

	timerButton++;

#ifdef reset_enable
	delay_PWM++;
	speed_PWM++;
#endif

#ifdef reset_disable
	delay_PWM[0]++;
	speed_PWM[0]++;
	delay_PWM[1]++;
	speed_PWM[1]++;
#endif
}


int main (void) {

	uint8_t 	chanel = 0;             // Номер канала
	uint8_t 	ch;
	uint16_t 	eeTime_flash;

	DDRB  = (0<<DDB5)   | (0<<DDB4)   | (0<<DDB3)   | (1<<DDB2)   | (1<<DDB1)   | (1<<DDB0);
	PORTB = (1<<PORTB5) | (1<<PORTB4) | (1<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0); 

	BLOK_RELE_ON;

#ifdef reset_enable
	if (KEY1) {key_button(); BLOK_RELE_OFF;}		// если в момент включения нажата кнопка KEY1 - переходим в режим программирования задержек времени
#endif

#ifdef reset_disable
	if (KEY1) {key_button(0); BLOK_RELE_OFF;}		// если в момент включения нажата кнопка KEY1 - переходим в режим программирования задержек времени, с указателем нажатия KEY1
	if (KEY2) {key_button(1); BLOK_RELE_OFF;}		// если в момент включения нажата кнопка KEY2 - переходим в режим программирования задержек времени, с указателем нажатия KEY2
#endif
	


	// Timer/Counter 0 initialization
	// Clock source: System Clock
	// Clock value: 150,000 kHz
	// Mode: Phase correct PWM top=0xFF
	// OC0A output: Non-Inverted PWM
	// OC0B output: Non-Inverted PWM
	// Timer Period: 3,4 ms
	// Output Pulse(s):
	// OC0A Period: 3,4 ms Width: 0 us
	// OC0B Period: 3,4 ms Width: 0 us

	//TCCR0A = (1<<COM0A1) | (0<<COM0A0) | (1<<COM0B1) | (0<<COM0B0) | (0<<WGM01) | (1<<WGM00);
	//TCCR0B = (0<<WGM02)  | (0<<CS02)   | (1<<CS01)   | (0<<CS00);
	TCCR0A=(1<<COM0A1) | (0<<COM0A0) | (1<<COM0B1) | (0<<COM0B0) | (1<<WGM01) | (1<<WGM00);
	TCCR0B=(0<<WGM02) | (0<<CS02) | (1<<CS01) | (0<<CS00);
	TCNT0  =  0x00;

	// Timer/Counter 0 Interrupt(s) initialization
	TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

	sei();

#ifdef reset_enable
	while(1) {
		eeTime_flash = eeprom_read_word(&eeTime[stupen_PWM]);

		if (delay_PWM <= eeTime_flash) {
			ch = stupen_enable;
			if (stupen_PWM == 1 || stupen_PWM == 3) {
				if (speed_PWM == eeTime_flash/brightness) {
					ch = speed_PWM_enable; speed_PWM = 0;}}
		}
		else {
			ch = stupen_disable;
			stupen_PWM++;
			delay_PWM = speed_PWM = 0;
			ledout = LED_PWM_1 = LED_PWM_2;
		}

		switch (stupen_PWM) {
			case 0:		if (ch == stupen_enable) LED_PWM_1 = LED_PWM_2 = 0x00; 
						break;
			
			case 1:		if (ch == speed_PWM_enable) {if (ledout != brightness) {LED_PWM_1 = LED_PWM_2 = (ledout*ledout) >> 8; ledout++;}}
						break;
			
			case 2:		if (ch == stupen_enable) LED_PWM_1 =  LED_PWM_2 = brightness;
						break;

			case 3:		if (ch == speed_PWM_enable) {if (ledout != 0x00) {LED_PWM_1 =  LED_PWM_2 = (ledout*ledout) >> 8; ledout--;}}
						break;
		}
		
		if (stupen_PWM == 4) BLOK_RELE_OFF;

		if (timerButton > 732 && flagStart == false) flagStart = true;
		if (RELE && flagStart) BLOK_RELE_OFF;
	}
#endif

#ifdef reset_disable
	while (1) {
		
		eeTime_flash = eeprom_read_word(&eeTime[chanel][stupen_PWM[chanel]]);

		if (delay_PWM[chanel] <= eeTime_flash) {											// сравниваем временную задержку таймера и записанное в ЕЕПРОМ значени
			ch = stupen_enable;																// если еще не достигли значение ЕЕПРОМ, то остаемся в текущей ступени
			if (stupen_PWM[chanel] == 1 || stupen_PWM[chanel] == 3) {						// если сейчас ступень 1 или 3 (розжиг или затухание, соответственно), то
				if (speed_PWM[chanel] == eeTime_flash/brightness[chanel]) {
					ch = speed_PWM_enable; speed_PWM[chanel] = 0;}}							// и скорость увеличение/уменьшения пропорциональна записанной в ЕЕПРОМ, то 
		}																					// разрешаем работу 1 и 3 ступеней
		else {																				// если таймер превысил задержку записанную в ЕЕПРОМ, то
			ch = stupen_disable;															// запрещаем работу всех ступеней и увеличиваем номер ступени на единицу.
			stupen_PWM[chanel]++;
			delay_PWM[chanel] = 0;
			speed_PWM[chanel] = 0;
			ledout[0] = LED_PWM_1;
			ledout[1] = LED_PWM_2;
		}											 
	
		switch (stupen_PWM[chanel]) {

			case 0:
					if (ch == stupen_enable) {chanel ? LED_PWM_2 = 0x00 : LED_PWM_1 = 0x00;}
					break;


			case 1:
					if (ch == speed_PWM_enable) {
						if (chanel == 0) {if (ledout[0] != brightness[chanel]) {LED_PWM_1 = (ledout[0]*ledout[0]) >> 8; ledout[0]++;}}
						else			 {if (ledout[1] != brightness[chanel]) {LED_PWM_2 = (ledout[1]*ledout[1]) >> 8; ledout[1]++;}}	
					}
					break;


			case 2:
					if (ch == stupen_enable) {chanel ? LED_PWM_2 = brightness[chanel] : LED_PWM_1 = brightness[chanel];}
					break;

			case 3:
					if (ch == speed_PWM_enable) {
						if (chanel == 0) {if (ledout[0] != 0x00) LED_PWM_1 = ledout[0]*ledout[0] >> 8; ledout[0]--;}
						else			 {if (ledout[1] != 0x00) LED_PWM_2 = ledout[1]*ledout[1] >> 8; ledout[1]--;}
					}
					break;


			case 4:
					chanel ? LED_PWM_2 = 0x00 : LED_PWM_1 = 0x00;
					break;

		}

		chanel = !chanel;
		if (stupen_PWM[0] == 4 && stupen_PWM[1] == 4) BLOK_RELE_OFF;

		if (timerButton > 732 && flagStart == false) flagStart = true;
		if (RELE && flagStart) BLOK_RELE_OFF;

	}
#endif
}
