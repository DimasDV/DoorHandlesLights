#ifdef CHANEL_1

    EEMEM uint16_t eeTime[4] = {0*FreqMulti, 10*FreqMulti, 5*FreqMulti, 15*FreqMulti};
	EEMEM uint8_t eeBrightness;
    uint8_t 	ledout;
    uint8_t 	stupen_PWM;			// Номер ступени [номер канала]
    uint8_t 	speed_PWM;			// Скорость изменения ШИМ [номер канала]
    uint16_t 	delay_PWM;			// Время работы ступени [номер канала]

	void setPWM() {
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
	}

    ISR (TIM0_OVF_vect) {
		timerButton++;
        delay_PWM++;
        speed_PWM++;
    }

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
			eeprom_update_word(&eeTime[nomer],uint16_t(timeProgEE *FreqMulti));
			nomer++;
		} while (nomer != 4);
		miganie_svetikom(5);
	}

    int main (void) {

		uint8_t 	ch;
		bool		tempBrihtnes = true;
		uint8_t 	timeProg = 0;
		uint16_t 	eeTime_flash;
		uint8_t 	brightness;

		DDRB  = (0<<DDB5)   | (0<<DDB4)   | (0<<DDB3)   | (1<<DDB2)   | (1<<DDB1)   | (1<<DDB0);
		PORTB = (1<<PORTB5) | (1<<PORTB4) | (1<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0); 

		BLOK_RELE_ON;

		if (KEY1) {
		key_button(); 
			setPWM(); 
			brightness = eeprom_read_byte(&eeBrightness);
			LED_PWM_1 = LED_PWM_2 = brightness;
			while (timeProg != 20) {
				_delay_ms(100);
				timeProg++;
				flagStart = false;
				if (KEY1) {
					_delay_ms(100);
					while (KEY1) {
						_delay_ms(20);
						timeProg = 0;
						LED_PWM_1 = LED_PWM_2 = brightness;
						tempBrihtnes ? brightness++ : brightness--;
						if ((brightness == 50 || brightness == 255) && ! flagStart) {
							for (uint8_t i=0; i<5; i++) {
								LED_PWM_1 = LED_PWM_2 = 255;
								_delay_ms(200);
								LED_PWM_1 = LED_PWM_2 = 0;
								_delay_ms(200);
							}
							flagStart = true;
						}
					}
					tempBrihtnes = ! tempBrihtnes;
				}	
			}
			eeprom_update_byte(&eeBrightness, brightness);
			TCCR0A = 0x00;
			TCCR0B = 0x00;
			cli();
			miganie_svetikom(11);
			BLOK_RELE_OFF;
		}		// если в момент включения нажата кнопка KEY1 - переходим в режим программирования задержек времени

		setPWM();

		while(1) {
			eeTime_flash 	= eeprom_read_word(&eeTime[stupen_PWM]);
			brightness		= eeprom_read_byte(&eeBrightness);

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
				
				default:	BLOK_RELE_OFF;
			}

			if (timerButton > 732 && flagStart == false) flagStart = true;
			if (RELE && flagStart) BLOK_RELE_OFF;
		}
	}
#endif