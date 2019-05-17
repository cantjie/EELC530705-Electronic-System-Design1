#include<REG51.h>
#include<absacc.h>
#define DELAY_TIMES 1

unsigned char G_timer0_count = 0;
unsigned char G_timer0_flag = 0;
unsigned char G_keycode = 0;
unsigned char G_key_pressing_flag = 0;
unsigned char G_setting_temperature = 60;
unsigned char G_measured_temperature = 0;
unsigned char G_LCD_table[2][16] = { {'s','e','t','t','i','n','g',':',0x20,0x20,0x20,0x20,'6','0',0xDF,'C'},
					{'m','e','a','s','u','r','e','d',':',0x20,0x20,0x20,'0','0',0xDF,'C'} };

void delay(unsigned char times);
void LCDWriteControl(unsigned char control_word);
void LCDWriteData(unsigned char data_word);
void LCDWriteString(unsigned char * str_ptr, unsigned short length);
void LCDInit();
void LCDRefresh();
unsigned char getKeycode(void);
void timerAndInterruptInit();
void setTemperature(unsigned char keycode);
void refreshLCDTable(unsigned char mode);



void main() 
{
	int i = 0;
	unsigned char keycode = 0x00;
	LCDInit();
	timerAndInterruptInit();
	
	while (1) {
		// scan keyboard every 50ms
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

		if (G_keycode) {
			setTemperature(keycode);
			G_keycode = 0;
		}
	}	
}


void delay(unsigned char times) 
{
	int i = 0, j = 0;
	for (i = 0; i < times; i++) {
		for (j = 0; j < 110; j++) {

		}
	}
}


void LCDWriteControl(unsigned char control_word) 
{
	XBYTE[0xA000] = control_word;
	delay(DELAY_TIMES);
}


void LCDWriteData(unsigned char data_word) 
{
	XBYTE[0xA002] = data_word;
	delay(DELAY_TIMES);
}


void LCDWriteString(unsigned char *str_ptr, unsigned short length) 
{
	short i = 0;
	for (i = 0; i < length; i++) {
		LCDWriteData(str_ptr[i]);
	}
}


void LCDInit() 
{
	LCDWriteControl(0x38); // 16*2��ʾ��5*7����8λ���ݿ�
	LCDWriteControl(0x0E); // ���ÿ�����ʾ������ʾ���
	LCDWriteControl(0x06); // дһ���ַ����ַָ��+1
	LCDWriteControl(0x01); // ��ʾ���㣬����ָ������0
	LCDWriteControl(0x80 + 0x01); // �������ݵ�ַָ��ӵ�һ����ʼ
	LCDWriteString(G_LCD_table[0], 16);
	LCDWriteControl(0x80 + 0x40);
	LCDWriteString(G_LCD_table[1], 16);
}


void LCDRefresh()
{
	LCDWriteControl(0x01);
	LCDWriteControl(0x80 + 0x01);
	LCDWriteString(G_LCD_table[0], 16);
	LCDWriteControl(0x80 + 0x40);
	LCDWriteString(G_LCD_table[1], 16);
}


unsigned char getKeycode(void) 
{
	//�洫����
	unsigned char line = 0x00;        //����
	unsigned char col = 0x00;		 //����
	unsigned char scancode = 0x01;    //��ɨ����
	unsigned char keycode;          //����
	XBYTE[0x8000] = 0xff;
	col = XBYTE[0x8000] & 0x0f;      //���ж˿ڶ�����λ����
	if (col == 0x00) {
		keycode = 0x00;
	}
	else
	{
		while ((scancode & 0x0f) != 0) //ȡscancode�ĵ���λ��û��Ϊȫ0��ѭ��
		{
			line = scancode;                //�к�
			XBYTE[0x8000] = scancode;     //���и�ɨ���룬��һ��Ϊ0x01
			if ((XBYTE[0x8000] & 0x0f) == col) //��ⰴ�����ڵ�������ѭ��
				break;
			scancode = scancode << 1;         //��ɨ��������һλ��ת��һ��
		}
		col = col << 4;                     //�������Ƶ�����λ
		keycode = col | line;
	}
	return keycode;
}


void timerAndInterruptInit()
{
	EA = 1;			// enable global interrupt

	TMOD = 0x11;	// set timer0 and timer1 to mode 1

	// (65535-X)*12/11.0592us = 50ms; X=19455;
	TH0 = 0x4B;		// load 19455D to timer0
	TL0 = 0xFF;
	TR0 = 1;		// start timer0
	ET0 = 1;		// enable timer0 interrupt

	// (65535-X)*12/11.0592us=2ms; X=63691��  
	//TH1 = 0xF8;		// load 63691 to timer1 ��2ms
	//TL1 = 0xCB;
	//TR1 = 1;		// start timer0
	//ET1 = 1;		// enable timer0 interrupt
}


void setTemperature(unsigned char keycode)
{
	switch (keycode)
	{
	case 0x14: //row 3 col 1
		G_setting_temperature = (G_setting_temperature == 100) ? 100 : G_setting_temperature + 1;
		break;
	case 0x24:
		G_setting_temperature = (G_setting_temperature == 0) ? 0 : G_setting_temperature - 1;
		break;
	case 0x44:
		G_setting_temperature = (G_setting_temperature + 10 >= 100) ? 100 : G_setting_temperature + 10;
		break;
	case 0x84:
		G_setting_temperature = (G_setting_temperature - 10 >= 100) ? 0 : G_setting_temperature - 10;
		break;
	default:
		break;
	}
	refreshLCDTable(0x01);
	LCDRefresh();
}


void refreshLCDTable(unsigned char mode) 
{
	if (mode & 0x01) {
		// refresh setting temperature
		G_LCD_table[0][13] = G_setting_temperature % 10 + 0x30;
		G_LCD_table[0][12] = (G_setting_temperature / 10) % 10 + 0x30;
		G_LCD_table[0][11] = (G_setting_temperature / 100) == 0 ? 0x20 : '1';
	}
	if (mode && 0x02) {
		// refresh measured temperature
		G_LCD_table[1][13] = G_measured_temperature % 10 + 0x30;
		G_LCD_table[1][12] = (G_measured_temperature / 10) % 10 + 0x30;
		G_LCD_table[1][11] = (G_measured_temperature / 100) == 0 ? 0x20 : '1';
	}
}


void timer0() interrupt 1 {
	TH0 = 0x4B;		// load 19455D to timer0, 50ms.
	TL0 = 0xFF;
	G_timer0_count++;
	G_timer0_flag = 1;
}