#include <REG51.H>
#include <absacc.h>

#define TIMER1_SECOND_COUNT 200
#define MEASURING_TIME 15
#define RATE_MATIPLIED_BY 4 // 4 = 60 / 15

#define MEASURING 0x01
#define NOT_MEASURE 0x00

unsigned char led_table[] = { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x08,0x40,0x79 };

unsigned char G_timer1_count = 0;
unsigned char G_timer1_flag = 0;
unsigned char G_key_pressing_flag = 0;
unsigned char G_keycode = 0;
unsigned char G_measure_flag = 0;
unsigned char G_seconds = 0;
unsigned long G_result = 0;
unsigned char G_result_times = 0;
unsigned char G_result_array[8] = { 0 };

unsigned char getKeycode() {
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

void timerInit() {
	EA = 1;
	TMOD = 0x15; // 0001 0101. timer0 in counter mode and mode 1; timer1 in timer mode and mode 1.
	
	// (65535-X)*12/11.0592us = 5ms;
	TH1 = 0xED;// X = 60927D 5ms
	TL1 = 0xFF;

	TR1 = 1;
	ET1 = 1;
}

void resultToArray() {
	G_result_array[7] = (G_result / 10000000) % 10;
	G_result_array[6] = (G_result / 1000000) % 10;
	G_result_array[5] = (G_result / 100000) % 10;
	G_result_array[4] = (G_result / 10000) % 10;
	G_result_array[3] = (G_result / 1000) % 10;
	G_result_array[2] = (G_result / 100) % 10;
	G_result_array[1] = (G_result / 10) % 10;
	G_result_array[0] = G_result % 10;
}

void startMeasure() {
	G_measure_flag = MEASURING;
	TH1 = 0xED;// X = 60927D 5ms
	TL1 = 0xFF; // reset Timer1 to avoid error.
	G_timer1_count = 0;

	TH0 = 0;
	TL0 = 0;

	TR0 = 1;
	ET0 = 1;
	G_seconds = 0;
	G_result = 0;
	resultToArray();
	G_result_times = 0;
}

void displayResult() {
	static unsigned char shift=0;
	shift += 1;
	shift %= 8;
	XBYTE[0x8000] = 0x01 << shift;
	XBYTE[0x9000] = led_table[G_result_array[shift]];
}

void stopMeasure() {
	TR0 = 0;
	ET0 = 0;

	G_result = TH0 << 8;
	G_result += TL0;
	G_result *= 4;
	G_result += G_result_times *4* 65536;
	G_result /= 60;
	resultToArray();

	G_measure_flag = NOT_MEASURE;
	//todo 
}


void main() {
	unsigned char keycode = 0x00;
	timerInit();
	
	for (;;) {
		// get keycode,per 50 ms
		if (G_timer1_count % 10 == 0) {
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
		}

		// if key pressed,start measure.press any key to start
		if (G_keycode) {
			G_result_array[0] = 12;
			
			if (NOT_MEASURE == G_measure_flag) {
				startMeasure();
			}
		}

		// 5ms passed display result
		if (G_timer1_flag) {
			displayResult();
			G_timer1_flag = 0;
		}

		if (MEASURING == G_measure_flag) {
			if (TIMER1_SECOND_COUNT == G_timer1_count) {
				G_seconds++;
				G_timer1_count = 0;
				if (MEASURING_TIME == G_seconds) {
					stopMeasure();
				}
			}
		}
	}
}


void timer0() interrupt 1{
	G_result_times++;
}

void timer1() interrupt 3{
	TH1 = 0xED;// X = 60927D 5ms
	TL1 = 0xFF;
	G_timer1_count++;
	G_timer1_flag = 1;
}