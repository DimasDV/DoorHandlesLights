
//#define bs_none				0
//#define bs_deBounce			1
//byte buttonState = bs_none;

/* 	
        if (buttonState == bs_none) {
			if (RELE) {
				buttonState = bs_deBounce;
				timerButton = 0;}
		}
		if (buttonState == bs_deBounce) {
			if (RELE) {
				if (timerButton > 150) BLOK_RELE_OFF;}
			else buttonState = bs_none;
		}
	


		if (RELE && flagStart) {
			if (buttonState == bs_none) timerButton = 0;
			buttonState = bs_deBounce;
			if (timerButton > 150) BLOK_RELE_OFF;
		}
		else {
			if (timerButton > 300 && flagStart == 0) {flagStart = 1;}
			buttonState = bs_none;
		}
*/