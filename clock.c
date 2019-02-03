#include <REG51.H>
#include <absacc.h>

#define TIMER0_SECOND_COUNT 20

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

// from 0 to 9, and blank, _, -, E
unsigned char code led_table[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x08,0x40,0x79 };

unsigned char G_timer0_count = 0;
unsigned char G_timer0_flag = 0;
unsigned char G_timer1_count = 0;
unsigned char G_timer1_flag = 0;

unsigned char G_keycode = 0;
unsigned char G_key_pressing_flag = 0;

unsigned char G_mode = 0;

unsigned char G_roling_shift = 0;

unsigned char G_setting_bit = 0;

unsigned char G_max_day[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

unsigned char G_time_array[8] = { 0,0,12,0,0,12,0,0 };
unsigned char G_date_array[12] = { 2,0,1,9,12,0,1,12,0,1,10,10 };

time G_current_time = { 2019,1,1,0,0,0,0};
time *G_pointer_current_time = &(G_current_time);

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
	return keycode;
}
 
/*
* correct the time after time changed
*/
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
			ptr->hour -= 24;
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

/*
* when 1s passes, call this function to increase the second
*/
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
void displayDateRoling() {
	unsigned char bit_to_show_in_date_array;
	unsigned char bit_to_show_in_eight_leds;
	if (G_timer1_count % 350 == 0) {
		G_roling_shift++;
		if (G_roling_shift == 12 * 3) {
			G_roling_shift = 0;
			G_mode = 0;
		}
	}
	bit_to_show_in_eight_leds = G_timer1_count % 8;
	bit_to_show_in_date_array = (G_roling_shift + bit_to_show_in_eight_leds) % 12;
	XBYTE[0x8000] = 0x80 >> bit_to_show_in_eight_leds;
	XBYTE[0x9000] = led_table[G_date_array[bit_to_show_in_date_array]];
}

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

unsigned char modeSwitch(unsigned char current_mode) {
	/*
		0 for displaying time, hh-mm-ss
		2 for setting time, hh-mm-ss , make the bit being changed blink
		3 for setting date, yyyymmdd,
		4 for setting clock.
		
		1 for showing date, yyyy-mm-dd,roling
	*/

	if (current_mode == 0) {
		return 1;
	}
	else
	{	
		if (current_mode == 1) {
			G_roling_shift = 0;
		}
		else {
			G_setting_bit = 0;
		}
		return (current_mode + 1) % 4 ;
		// todo, make this 5 later
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

void dealWithKeyPressed(unsigned char keycode) {
	if (keycode == 0x14) { // row 3 col 1 
		G_mode = modeSwitch(G_mode);
	}
	else {
		if (keycode == 0x24) { //row 3 col 2
			if (G_mode == 2) {
				G_setting_bit = (G_setting_bit + 1) % 2;
			}
			else {
				if (G_mode == 3) {
					G_setting_bit = (G_setting_bit + 1) % 8;  // will compiler optimize %2, %8 ? to &0x01,0x07 ?
				}
			}
		}
		else {
			if (keycode == 0x44) { //row 3 col 3
				if (G_mode == 2) {
					setTime(G_pointer_current_time, G_setting_bit);
				}
				else {
					if (G_mode == 3) {
						setDate(G_pointer_current_time, G_setting_bit);
					}
				}
				timeToArray(G_pointer_current_time);
				dateToArray(G_pointer_current_time);
			}
		}
		if (G_mode == 1) {
			G_roling_shift = 0;
			G_mode = 0;
		}
	}
}

/*
*	excute different routine work in different mode
*/
void excute(unsigned char mode) {
	switch (mode)
	{
	case 0:
		displayTime();
		break;
	case 1:
		displayDateRoling();
		break;
	case 2:
		displayTimeBlink(G_setting_bit);
		break;
	case 3:
		displayDateBlink(G_setting_bit);
		break;
	default:
		XBYTE[0x8000] = 0x40 ;
		XBYTE[0x9000] = led_table[mode];
		break;
	}
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
	unsigned char keycode = 0;
	unsigned char key_input_break = 0;

	timerAndInterruptInit();
	
	while(1) {
		
		//1s has passed
		if (G_timer0_count == TIMER0_SECOND_COUNT) {
			timeAutoIncrement(G_pointer_current_time);
			G_timer0_count = 0;
		}

		//50ms passed, check if any key being pressed
		if (G_timer0_flag) {
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
			excute(G_mode);
			G_timer1_flag = 0;
		}

		//if key pressed
		if (G_keycode) {
			dealWithKeyPressed(G_keycode);
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
}

void timer1() interrupt 3{
	TH1 = 0xF8;		// load 63691 to timer1 ，2ms
	TL1 = 0xCB;
	G_timer1_count++;
	G_timer1_flag = 1;
}
