#ifdef CHANEL_2
    EEMEM uint16_t eeTime[2][4] = {{0*FreqMulti,10*FreqMulti,5*FreqMulti,15*FreqMulti},{0*FreqMulti,5*FreqMulti,10*FreqMulti,15*FreqMulti}};   // Хранится время задержки включения, время плавного включения, время работы и плавного выключения каналов
	//uint8_t		brightness[2] = {255, 255};
	uint8_t 	ledout[2];
	uint8_t 	stupen_PWM[2];			// Номер ступени [номер канала]
	uint8_t 	speed_PWM[2];			// Скорость изменения ШИМ [номер канала]
	uint16_t 	delay_PWM[2];			// Время работы ступени [номер канала]

    ISR (TIM0_OVF_vect) {
        timerButton++;
        delay_PWM[0]++;
        speed_PWM[0]++;
        delay_PWM[1]++;
        speed_PWM[1]++;
    }

    void miganie_svetikom (uint8_t x, uint8_t ch) {
		for (uint8_t z=0; z<=x; z++) {
			LED_XOR(ch);
			_delay_ms(300);
		}
	}

	void key_button (uint8_t ch) {											// функция изменения и записи в ЕЕПРОМ временных задержек (ch - указатель на то какая кнопка была нажата)

		uint8_t timeProg;
		uint8_t timeProgEE;
		uint8_t nomer = 0;

		//nomer = 0;

		do {
			miganie_svetikom(5,ch -4);										// моргаем 3 раза соответствующим каналом 
							
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

    int main (void) {

        uint8_t 	chanel = 0;             // Номер канала
        uint8_t 	ch;
        uint16_t 	eeTime_flash;

        DDRB  = (0<<DDB5)   | (0<<DDB4)   | (0<<DDB3)   | (1<<DDB2)   | (1<<DDB1)   | (1<<DDB0);
        PORTB = (1<<PORTB5) | (1<<PORTB4) | (1<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0); 

        BLOK_RELE_ON;

        if (KEY1) {key_button(4); BLOK_RELE_OFF;}		// если в момент включения нажата кнопка KEY1 - переходим в режим программирования задержек времени, с указателем нажатия KEY1
        //if (KEY2) {key_button(5); BLOK_RELE_OFF;}		// если в момент включения нажата кнопка KEY2 - переходим в режим программирования задержек времени, с указателем нажатия KEY2

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

        while (1) {
            
            eeTime_flash = eeprom_read_word(&eeTime[chanel][stupen_PWM[chanel]]);

            if (delay_PWM[chanel] <= eeTime_flash) {											// сравниваем временную задержку таймера и записанное в ЕЕПРОМ значени
                ch = stupen_enable;																// если еще не достигли значение ЕЕПРОМ, то остаемся в текущей ступени
                if (stupen_PWM[chanel] == 1 || stupen_PWM[chanel] == 3) {						// если сейчас ступень 1 или 3 (розжиг или затухание, соответственно), то
                    if (speed_PWM[chanel] == eeTime_flash/255) {
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

                case 0:     if (ch == stupen_enable) {chanel ? LED_PWM_2 = 0x00 : LED_PWM_1 = 0x00;}
                            break;

                case 1:     if (ch == speed_PWM_enable) {
                                if (chanel == 0) {if (ledout[0] != 255) {LED_PWM_1 = (ledout[0]*ledout[0]) >> 8; ledout[0]++;}}
                                else			 {if (ledout[1] != 255) {LED_PWM_2 = (ledout[1]*ledout[1]) >> 8; ledout[1]++;}}}
                            break;

                case 2:
                        if (ch == stupen_enable) {chanel ? LED_PWM_2 = 255 : LED_PWM_1 = 255;}
                        break;

                case 3:     if (ch == speed_PWM_enable) {
                                if (chanel == 0) {if (ledout[0] != 0x00) LED_PWM_1 = ledout[0]*ledout[0] >> 8; ledout[0]--;}
                                else			 {if (ledout[1] != 0x00) LED_PWM_2 = ledout[1]*ledout[1] >> 8; ledout[1]--;}}
                            break;
                        
                case 4:     chanel ? LED_PWM_2 = 0x00 : LED_PWM_1 = 0x00;
                            break;

            }

            chanel = !chanel;
            if (stupen_PWM[0] == 4 && stupen_PWM[1] == 4) BLOK_RELE_OFF;

            if (timerButton > 732 && flagStart == false) flagStart = true;
            if (RELE && flagStart) BLOK_RELE_OFF;

        }
    }
#endif