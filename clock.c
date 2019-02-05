#include <REG51.H>
#include <absacc.h>

#define TIMER0_SECOND_COUNT 20
#define CLOCK_MUSIC_NUMBER 1  // below 10 //todo
#define MAX_MUSIC_LENGTH 40
#define TIMER0_BEAT_LENGTH 4  // delay time of 1/4 beat ,

sbit beep_pin = P1 ^ 6;

typedef struct Time
{
	unsigned int year;
	unsigned char month;
	unsigned char day;

	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	unsigned char isLeap;
} time;

typedef struct Clock
{
	unsigned char hour;
	unsigned char minute;
	unsigned char music; // set to 0 to unset the clock. set to other number to choose music
} clock;

typedef struct Music {
	unsigned char len;
	//todo change MAX_MUSIC_LENGTH, 100 is improper. 
	unsigned char score[MAX_MUSIC_LENGTH];
	unsigned char beat_len[MAX_MUSIC_LENGTH];
	unsigned char playing_index;
} music;

// from 0 to 9, and blank, _, -, E
unsigned char code led_table[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x08,0x40,0x79 };

unsigned char G_timer0_count = 0;
unsigned char G_timer0_music_count = 0;
unsigned char G_timer0_flag = 0;
unsigned char G_timer1_count = 0;
unsigned char G_timer1_flag = 0;

unsigned char G_keycode = 0;
unsigned char G_key_pressing_flag = 0;

unsigned char G_mode = 0;

unsigned char G_rolling_shift = 0;

unsigned char G_setting_bit = 0;

//https://blog.csdn.net/shy_0001/article/details/78513544
unsigned char G_beat_length = 4;
unsigned char G_TIMER1_TH = 0x00;
unsigned char G_TIMER1_TL = 0x00;
music *G_music_playing_ptr = 0x00;
music music_mother = {
	34,
	{16,15,13,15,21,16,15,16, 13,15,16,15,13,12,11,6,15,13,12, 12,13,15,15,16,13,12,11, 15,13,12,11,6,11,5},
	{3, 1, 2 ,2, 2, 2, 1, 4,  2, 1, 1, 2, 1, 1, 1, 1, 1,1, 4,  3, 1, 2, 1, 1, 3, 1, 4,
	  3, 1, 1, 1, 1,1, 4},
	0
};

//unsigned char G_score_to_freq[27] = {
//	262,294,330,349,392,440,494,0,0,0,
//	523,587,659,698,784,880,988,0,0,0,
//	1046,1175,1318,1397,1568,1760,1967
//};

/*	(65535 - X) * 12 / 11.0592us = yms;   
	65535-X = y*11.0592/12 *1000
	X = 65535 - y*11059.2/12  
	yms = 1000/freq
	 X = 65535- 11059200/12/freq
	pay attention to the freq here, it should be double of the frequency.
	at half of T, we'd change the pin voltage.
	F920	F9DF	FA8A	FAD6	FB67	FBE7	FC5A
	FC8D	FCED	FD43	FD6A	FDB3	FDF3	FE2C
	FE46	FE76	FEA1	FEB5	FED9	FEF9	FF14
	*/
unsigned char G_score_to_TH[27] = {
	0xF9,0xF9,0xFA,0xFA,0xFB,0xFB,0xFC,0x00,0x00,0x00,
	0xFC,0xFC,0xFD,0xFD,0xFD,0xFD,0xFE,0x00,0x00,0x00,
	0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF
};

unsigned char G_score_to_TL[27] = {
	0x20,0xDF,0x8A,0xD6,0x67,0xE7,0x5A,0x00,0x00,0x00,
	0x8D,0xED,0x43,0x6A,0xB3,0xF3,0x2C,0x00,0x00,0x00,
	0x46,0x76,0xA1,0xB5,0xD9,0xF9,0x14
};



unsigned char G_max_day[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
unsigned char G_time_array[8] = { 0,0,12,0,0,12,0,0 };
unsigned char G_date_array[12] = { 2,0,1,9,12,0,1,12,0,1,10,10 };

unsigned char G_clock_array[8] = { 0,10,10,0,0,12,0,0 }; //first bit for setting music, last 5 bits for setting clock time;

time G_current_time = { 2019,1,1,0,0,0,0};
time *G_pointer_current_time = &(G_current_time);
clock G_clock = { 0,0,0 };
clock *G_pointer_clock = &(G_clock);

unsigned char G_clock_hour = 0;
unsigned char G_clock_minute = 0;
unsigned char G_clock_music = 0;

void timeToArray(time *ptr);

/********************/

unsigned char isLeap(time *ptr) {
	if ((ptr->year % 400 == 0) || ((ptr->year % 4 == 0) && (ptr->year % 100 != 0))) {
		return 1;
	}
	else {
		return 0;
	}
}

// this function has no use.
void delayByMs(unsigned char time) {
	static unsigned char i;
	for (; time > 0; time--);
	for (i = 0; i < 440; i++);
}

unsigned char getKeycode(void) {
	unsigned char line = 0x00;        /*行码*/
	unsigned char col = 0x00;		 /*列码*/
	unsigned char scancode = 0x01;    /*行扫描码*/
	unsigned char keycode;          /*键号*/
	XBYTE[0x8000] = 0xff;
	col = XBYTE[0x8000] & 0x0f;      /*从列端口读入四位列码*/
	if (col == 0x00) {
		keycode = 0x00;
	}
	else
	{
		while ((scancode & 0x0f) != 0) /*取scancode的低四位，没变为全0，循环*/
		{
			line = scancode;                /*行号*/
			XBYTE[0x8000] = scancode;     /*给行赋扫描码，第一行为0x01*/
			if ((XBYTE[0x8000] & 0x0f) == col) /*检测按键所在的行跳出循环*/
				break;
			scancode = scancode << 1;         /*行扫描码左移一位，转下一行*/
		}
		col = col << 4;                     /*把行码移到高四位*/
		keycode = col | line;
	}
	//XBYTE[0x8000] = 0x00;
	return keycode;
}
 
//correct the time after time changed
time *timeCorrection(time *ptr,unsigned char setting_mode) {
	if (!setting_mode) {
		if (ptr->second > 59) {
			ptr->second = 0;
			ptr->minute++;
		}

		if (ptr->minute >= 60) {
			ptr->minute = 0;
			ptr->hour++;
		}

		if (ptr->hour >= 24) {
			ptr->hour = 0;
			ptr->day++;
		}

		if (ptr->day > G_max_day[ptr->month - 1]) {
			ptr->day = 1;
			ptr->month++;
		}

		if (ptr->month > 12) {
			ptr->month = 1;
			ptr->year++;
			if (isLeap(ptr)) {
				ptr->isLeap = 1;
				G_max_day[1] = 29;
			}
			else {
				ptr->isLeap = 0;
				G_max_day[1] = 28;
			}
		}
	}
	else {
		if (ptr->second >= 60) {
			ptr->second -= 60;
		}

		if (ptr->minute >= 60) {
			ptr->minute -= 60;
		}

		if (ptr->hour >= 24) {
			ptr->hour -= 20;
		}

		if (ptr->day > G_max_day[ptr->month - 1]) {
			ptr->day = 1;
		}

		if (ptr->month > 12) {
			ptr->month = 1;
		}

		if (isLeap(ptr)) {
			ptr->isLeap = 1;
			G_max_day[1] = 29;
		}
		else {
			ptr->isLeap = 0;
			G_max_day[1] = 28;
		}
	}
	return ptr;
}

clock *clockCorrection(clock *clk) {
	if (clk->minute >= 60) {
		clk->minute -= 60;
	}

	if (clk->hour >= 24) {
		clk->hour -= 20;
	}
	return clk;
}

void timeToArray(time *ptr) {
	G_time_array[7] = ptr->second % 10;
	G_time_array[6] = ptr->second / 10;
	G_time_array[4] = ptr->minute % 10;
	G_time_array[3] = ptr->minute / 10;
	G_time_array[1] = ptr->hour % 10;
	G_time_array[0] = ptr->hour / 10;
}

void dateToArray(time *ptr) {
	unsigned int temp = 0;
	G_date_array[9] = ptr->day % 10;
	G_date_array[8] = ptr->day / 10;	

	G_date_array[6] = ptr->month % 10;
	G_date_array[5] = ptr->month / 10;
	
	G_date_array[0] = ptr->year / 1000;
	G_date_array[1] = (ptr->year % 1000) / 100;
	G_date_array[2] = (ptr->year % 100) / 10;
	G_date_array[3] = ptr->year % 10;
}

void clockToArray(clock *clk) {
	G_clock_array[7] = clk->minute % 10;
	G_clock_array[6] = clk->minute / 10;
	G_clock_array[4] = clk->hour % 10;
	G_clock_array[3] = clk->hour / 10;
	G_clock_array[0] = clk->music;
}

// when 1s passes, call this function to increase the second
time *timeAutoIncrement(time *ptr) {
	ptr->second++;
	timeCorrection(ptr, 0);																							   
	timeToArray(ptr);
	dateToArray(ptr);
	return ptr;	
}

//call this function every timer1 break in mode 0
void displayTime() {
	unsigned char shift;
	shift = G_timer1_count % 8;
	XBYTE[0x8000] = 0x80 >> shift;
	XBYTE[0x9000] = led_table[G_time_array[shift]];
}

// mode 1
void displayDateRolling() {
	unsigned char bit_to_show_in_date_array;
	unsigned char bit_to_show_in_eight_leds;
	if (G_timer1_count % 350 == 0) {
		G_rolling_shift++;
		if (G_rolling_shift == 12 * 3) {
			G_rolling_shift = 0;
			G_mode = 0;
		}
	}
	bit_to_show_in_eight_leds = G_timer1_count % 8;
	bit_to_show_in_date_array = (G_rolling_shift + bit_to_show_in_eight_leds) % 12;
	XBYTE[0x8000] = 0x80 >> bit_to_show_in_eight_leds;
	XBYTE[0x9000] = led_table[G_date_array[bit_to_show_in_date_array]];
}

// shift_bit is in either 0 or 1
void displayTimeBlink(unsigned char shift_bit){
	unsigned char shift;
	unsigned char blink = 0;
	if ((G_timer1_count & 0x40) == 0x40) {
		blink ^= 0x01;
	}
	shift = G_timer1_count % 8;
	XBYTE[0x8000] = 0x80 >> shift;
	if (shift_bit == 0) {
		if (shift == 3 || shift == 4) {
			XBYTE[0x9000] = blink ? led_table[G_time_array[shift]] : 0x00;
		}
		else
		{
			XBYTE[0x9000] = led_table[G_time_array[shift]];
		}
	}
	else
	{
		if (shift_bit == 1) {
			if (shift == 0 || shift == 1) {
				XBYTE[0x9000] = blink ? led_table[G_time_array[shift]] : 0x00;
			}
			else
			{
				XBYTE[0x9000] = led_table[G_time_array[shift]];
			}
		}
	}
	
}

//shift_bit is between 0 to 7
void displayDateBlink(unsigned char shift_bit) {
	//unsigned char G_date_array[12] = { 2,0,1,9,12,0,1,12,0,1,10,10 };
	unsigned char shift_in_leds;
	unsigned char shift_in_array;
	unsigned char blink = 0;
	if ((G_timer1_count & 0x40) == 0x40) {
		blink ^= 0x01;
	}
	shift_in_leds = G_timer1_count % 8;
	switch (shift_in_leds)
	{
	case 0:case 1:case 2: case 3:
		shift_in_array = shift_in_leds;
		break;
	case 4:case 5:
		shift_in_array = shift_in_leds + 1;
		break;
	case 6:case 7:
		shift_in_array = shift_in_leds + 2;
		break;
	default:
		break;
	}
	XBYTE[0x8000] = 0x80 >> shift_in_leds;
	if (shift_in_leds == 7 - shift_bit) {
		XBYTE[0x9000] = blink ? led_table[G_date_array[shift_in_array]] : 0x00;
	}
	else {
		XBYTE[0x9000] = led_table[G_date_array[shift_in_array]];
	}
}

//shift_bit is between 0 to 4; 0&1 for minute, 2&3 for hour, 4 for music or unset;
void displayClockBlink(unsigned char shift_bit) {
	unsigned char blink = 0;
	unsigned char shift = 0; // bit to show
	unsigned char blink_shift = 0; // bit to blink
	if ((G_timer1_count & 0x40) == 0x40) {
		blink ^= 0x01;
	}
	shift = G_timer1_count % 8;

	switch (shift_bit)
	{
	case 0: case 1:
		blink_shift = 7 - shift_bit;
		break;
	case 2: case 3:
		blink_shift = 6 - shift_bit;
		break;
	case 4:
		blink_shift = 0;
		break;
	default:
		break;
	}
	XBYTE[0x8000] = 0x80 >> shift;
	if (blink_shift == shift) {
		XBYTE[0x9000] = blink ? led_table[G_clock_array[shift]] : 0x00;
	}
	else {
		XBYTE[0x9000] = led_table[G_clock_array[shift]];
	}
}

unsigned char modeSwitch(unsigned char current_mode) {
	/*
		0 for displaying time, hh-mm-ss
		2 for setting time, hh-mm-ss , make the bit being changed blink
		3 for setting date, yyyymmdd,
		4 for setting clock.
		5 for stopwatch mode.
		6 for countdown timer.

		1 for showing date, yyyy-mm-dd,rolling
	*/

	if (current_mode == 0) {
		return 1;
	}
	else
	{	
		if (current_mode == 1) {
			G_rolling_shift = 0;
		}
		else {
			G_setting_bit = 0;
		}
		return (current_mode + 1) % 7 ;
	}
}

time *setTime(time *ptr, unsigned char shift_bit) {
	switch (shift_bit)
	{
	case 0:
		ptr->minute++;
		break;
	case 1:
		ptr->hour++;
		break;	
	default:
		break;
	}
	ptr->second = 0;
	return timeCorrection(ptr, 1);
}

time *setDate(time *ptr, unsigned char shift_bit) {
	switch (shift_bit)
	{
	case 0:
		ptr->day++;
		break;
	case 1:
		ptr->day += 10;
		break;
	case 2:
		ptr->month++;
		break;
	case 3:
		ptr->month += 10;
		break;
	case 4:
		ptr->year++;
		break;
	case 5:
		ptr->year += 10;
		break;
	case 6:
		ptr->year += 100;
		break;
	case 7:
		ptr->year += 1000;
		break;
	default:
		break;
	}
	return timeCorrection(ptr, 1);
}

clock *setClock(clock *clk, unsigned char shift_bit) {
	switch (shift_bit)
	{
	case 0:
		clk->minute++;
		break;
	case 1:
		clk->minute += 10;
		break;
	case 2:
		clk->hour++;
		break;
	case 3:
		clk->hour += 10;
		break;
	case 4:
		clk->music = clk->music >= CLOCK_MUSIC_NUMBER ? 0 : (clk->music + 1);
		break;
	default:
		break;
	}
	return clockCorrection(clk);
}

unsigned char isTimeEqualsClock(time *ptr, clock *clk) {
	if (ptr->second == 0) {
		if ((ptr->minute == clk->minute) && (ptr->hour == clk->hour)) {
			return 1;
		}
	}
	return 0;
}

void dealWithKeyPressed(unsigned char keycode) {
	if (keycode == 0x14) { // row 3 col 1 
		G_mode = modeSwitch(G_mode);
	}
	else {
		if (keycode == 0x24) { //row 3 col 2
			switch (G_mode)
			{
			case 2:
				G_setting_bit = (G_setting_bit + 1) % 2;
				break;
			case 3:
				G_setting_bit = (G_setting_bit + 1) % 8;  // will compiler optimize %2, %8 ? to &0x01,0x07 ?
				break;
			case 4:
				G_setting_bit = (G_setting_bit + 1) % 5;
				break;
			default:
				break;
			}
		}
		else {
			if (keycode == 0x44) {
				switch (G_mode)
				{
				case 2:
					setTime(G_pointer_current_time, G_setting_bit);
					timeToArray(G_pointer_current_time);
					break;
				case 3:
					setDate(G_pointer_current_time, G_setting_bit);
					dateToArray(G_pointer_current_time);
					break;
				case 4:
					setClock(G_pointer_clock,G_setting_bit);
					clockToArray(G_pointer_clock);
					break;
				default:
					break;
				}
			}
		}
		if (G_mode == 1) {
			G_rolling_shift = 0;
			G_mode = 0;
		}
	}
}

//display different routine work in different mode
void display(unsigned char mode) {
	switch (mode)
	{
	case 0:
		displayTime();
		break;
	case 1:
		displayDateRolling();
		break;
	case 2:
		displayTimeBlink(G_setting_bit);
		break;
	case 3:
		displayDateBlink(G_setting_bit);
		break;
	case 4:
		displayClockBlink(G_setting_bit);
		break;
	default:
		XBYTE[0x8000] = 0x40 ;
		XBYTE[0x9000] = led_table[mode];
		break;
	}
}

void startPlayMusic(unsigned char music_num) {

	switch (music_num)
	{
	case 1:
		G_music_playing_ptr = &music_mother;
		break;
		//todo add new music
	default:
		break;
	}
	G_music_playing_ptr->playing_index = 0;
	G_beat_length = G_music_playing_ptr->beat_len[0];
	G_TIMER1_TH = G_score_to_TH[G_music_playing_ptr->score[0] - 1];
	G_TIMER1_TL = G_score_to_TL[G_music_playing_ptr->score[0] - 1];
	G_timer0_music_count = 0;
}

void continuePlayMusic() {
	G_music_playing_ptr->playing_index >= G_music_playing_ptr->len ? G_music_playing_ptr->playing_index = 0 : G_music_playing_ptr->playing_index++;

	G_beat_length = G_music_playing_ptr->beat_len[G_music_playing_ptr->playing_index];
	G_TIMER1_TH = G_score_to_TH[G_music_playing_ptr->score[G_music_playing_ptr->playing_index] - 1];
	G_TIMER1_TL = G_score_to_TL[G_music_playing_ptr->score[G_music_playing_ptr->playing_index] - 1];
}

void timerAndInterruptInit(){
	EA = 1;			// enable global interrupt

	TMOD = 0x11;	// set timer0 and timer1 to mode 1

	// (65535-X)*12/11.0592us = 50ms; X=19455;
	TH0 = 0x4B;		// load 19455D to timer0
	TL0 = 0xFF;
	TR0 = 1;		// start timer0
	ET0 = 1;		// enable timer0 interrupt

	// (65535-X)*12/11.0592us=2ms; X=63691；  
	TH1 = 0xF8;		// load 63691 to timer1 ，2ms
	TL1 = 0xCB;
	TR1 = 1;		// start timer0
	ET1 = 1;		// enable timer0 interrupt
}

void main() {
	unsigned char keycode = 0x00;
	unsigned char key_input_break = 0x00;

	timerAndInterruptInit();


	//startPlayMusic(1);

	while(1) {
		
		//1s has passed, change time and check whether it's the clock time;
		if (G_timer0_count == TIMER0_SECOND_COUNT) {
			timeAutoIncrement(G_pointer_current_time);
			G_timer0_count = 0;

			if (G_pointer_clock->music != 0) {
				if (isTimeEqualsClock(G_pointer_current_time, G_pointer_clock) == 1) {
					startPlayMusic(G_pointer_clock->music);
				}
			}
		}

		//play music
		if (G_music_playing_ptr) {    
			if (G_timer0_music_count == TIMER0_BEAT_LENGTH * G_beat_length) {
				G_timer0_music_count = 0;
				continuePlayMusic();
			 }
		}

		//50ms passed, check if any key being pressed
		if (G_timer0_flag) {
		//todo. why if the next line is used instead of the former line, the program wont work properly.
		//if ((G_timer0_count & 0x01) == 0x01) {
			keycode = getKeycode();
			if (keycode) {
				if (G_key_pressing_flag) {
					G_keycode = 0;
				}
				else {
					G_key_pressing_flag = 1;
					G_keycode = keycode;
				}
			}
			else {
				G_key_pressing_flag = 0;
			}
			G_timer0_flag = 0;
		}

		// 2ms passed , excute routine work(display next 7-seg-led)
		if (G_timer1_flag) {
			display(G_mode);
			G_timer1_flag = 0;
		}

		//if key pressed
		if (G_keycode) {
			if (G_music_playing_ptr) {
				// if music is playing, then only to stop it.
				G_music_playing_ptr = 0x00;
				beep_pin = 1;
			}
			else{
				dealWithKeyPressed(G_keycode);
			}
			G_keycode = 0;
		}
	}
	// never reach here;
	return ;
}

void timer0() interrupt 1 {
	TH0 = 0x4B;		// load 19455D to timer0, 50ms.
	TL0 = 0xFF;
	G_timer0_count++;
	G_timer0_flag = 1;
	if (G_music_playing_ptr) {
		G_timer0_music_count++;
	}
}

void timer1() interrupt 3{
	//todo music, TH,TL
	if (G_music_playing_ptr) {
		TH1 = G_TIMER1_TH;
		TL1 = G_TIMER1_TL;
		beep_pin = !beep_pin;
	}
	else {
		TH1 = 0xF8;		// load 63692 to timer1 ，about 2ms 
		TL1 = 0xCC;		// in stopwatch mode, the time displayed is about 3 seconds faster than real time per minute
	}
	G_timer1_count++;
	G_timer1_flag = 1;
}
 