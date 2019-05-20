#include<REG51.h>
#include<absacc.h>

#define LCD_DELAY_TIMES 1
#define HOW_MANY_TIMER0_BE_SECOND 100
// todo about PID
#define PID_K_P 0.01
#define PID_K_I 0.001
#define PID_K_D 0.05
#define TEMPERATURE_DIFFERENCE_THRESHOLD 500

unsigned char G_led_ref[] = { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x08,0x40,0x79 };
unsigned char G_led_table[4] = {0};
unsigned char G_timer0_count = 0;
unsigned char G_timer0_flag = 0; 
unsigned char G_keycode = 0;
unsigned char G_key_pressing_flag = 0;
unsigned int G_setting_temperature = 3000;
unsigned int G_measured_temperature = 0;
unsigned char G_LCD_table[2][16] = { {'s','e','t','t','i','n','g',':',0x20,0x20,'3','0','.','0',0xDF,'C'},
					{'m','e','a','s','u','r','e','d',':',0x20,'0','0','.','0',0xDF,'C'} };
long G_pid_sum = 0;
int G_pid_difference = 0;
int G_pid_last_temperature_difference = 0;
unsigned int G_duty_cycle = HOW_MANY_TIMER0_BE_SECOND;

sbit pin_sclk = P1 ^ 2;
sbit pin_ADC_dataout = P1 ^ 3;
sbit pin_ADC_cs = P1 ^ 4;
sbit pin_PWM_out = P1 ^ 7;

void delay(unsigned char times);
void LCDWriteCommand(unsigned char control_word);
void LCDWriteData(unsigned char data_word);
void LCDWriteString(unsigned char * str_ptr, unsigned short length);
void LCDInit();
void LCDRefresh();
unsigned int ADCRead();
unsigned int ADCToTemperature(unsigned int ADC_data);
void ADCToLEDTable(unsigned int ADC_data);
void dutyCycleToLEDTable();
void LEDDisplay();
unsigned char getKeycode(void);
void timerAndInterruptInit();
void setTemperature(unsigned char keycode);
void refreshLCDTable(unsigned char mode);
void PIDInit();
void PIDupdate();
unsigned char getDutyCycle();
void PWMoutput();


void main()
{
	int i = 0;
	unsigned char keycode = 0x00;
	unsigned char key_scan_flag = 0;
	unsigned int ADC_data = 0;
	LCDInit();
	timerAndInterruptInit();
	ADCRead(); // ADC init

	while (1) {
		// scan keyboard every 50ms
		if ((0 == G_timer0_count % 5) && (0 == key_scan_flag)) {
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
			key_scan_flag = 1;
		}
		else {
			key_scan_flag = 0;
		}

		// set temperature
		if (G_keycode) {
			setTemperature(keycode);
			G_keycode = 0;
			PIDInit();
		}

		if (1 == G_timer0_flag) {
			LEDDisplay();
			G_timer0_flag = 0;
		}

		// 1s passed
		if (HOW_MANY_TIMER0_BE_SECOND == G_timer0_count) {
			// first, get the measured temperature.
			ADC_data = ADCRead();
			G_measured_temperature = ADCToTemperature(ADC_data);
			//ADCToLEDTable(ADC_data);
			dutyCycleToLEDTable();
			refreshLCDTable(0x01 | 0x02);
			LCDRefresh();
			G_duty_cycle = getDutyCycle();
			PWMoutput();
			G_timer0_count = 0;
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


void LCDWriteCommand(unsigned char control_word)
{
	XBYTE[0xA000] = control_word;
	delay(LCD_DELAY_TIMES);
}


void LCDWriteData(unsigned char data_word)
{
	XBYTE[0xA002] = data_word;
	delay(LCD_DELAY_TIMES);
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
	LCDWriteCommand(0x38); // 16*2显示，5*7点阵，8位数据口
	LCDWriteCommand(0x0E); // 设置开启显示，不显示光标
	LCDWriteCommand(0x06); // 写一个字符后地址指针+1
	LCDWriteCommand(0x01); // 显示清零，数据指针清零0
	LCDWriteCommand(0x80 + 0x01); // 设置数据地址指针从第一个开始
	LCDWriteString(G_LCD_table[0], 16);
	LCDWriteCommand(0x80 + 0x40);
	LCDWriteString(G_LCD_table[1], 16);
}


void LCDRefresh()
{
	LCDWriteCommand(0x01);
	LCDWriteCommand(0x80 + 0x01);
	LCDWriteString(G_LCD_table[0], 16);
	LCDWriteCommand(0x80 + 0x40);
	LCDWriteString(G_LCD_table[1], 16);
}


unsigned int ADCRead() 
{
	unsigned int ADC_data = 0;
	unsigned char i = 0;
	pin_ADC_cs = 0;
	for (i = 0; i < 10; i++)
	{
		pin_sclk = 0;
		pin_sclk = 1; // 上升沿读数据
		ADC_data |= (pin_ADC_dataout & 0x01);
		ADC_data <<= 1;
		delay(1);
	}
	ADC_data >>= 1;
	pin_ADC_cs = 1;//片选禁止
	return ADC_data;
}


unsigned int ADCToTemperature(unsigned int ADC_data)
{
	//return ADC_data * 1024 * 10000;
	return ADC_data * 10;
}


void ADCToLEDTable(unsigned int ADC_data)
{
	short i = 0;
	for (i = 0; i < 4; i++) {
		G_led_table[i] = ADC_data % 10;
		ADC_data /= 10;
	}
}


void dutyCycleToLEDTable()
{
	short i = 0;
	for (i = 0; i < 4; i++) {
		G_led_table[i] = G_duty_cycle % 10;
		G_duty_cycle /= 10;
	}
	
}


void LEDDisplay() {
	static unsigned char shift = 0;
	shift += 1;
	shift %= 4;
	XBYTE[0x8000] = 0x01 << shift;
	XBYTE[0x9000] = G_led_ref[G_led_table[shift]];
}


unsigned char getKeycode(void)
{
	//祖传代码
	unsigned char line = 0x00;        //行码
	unsigned char col = 0x00;		 //列码
	unsigned char scancode = 0x01;    //行扫描码
	unsigned char keycode;          //键号
	XBYTE[0x8000] = 0xff;
	col = XBYTE[0x8000] & 0x0f;      //从列端口读入四位列码
	if (col == 0x00) {
		keycode = 0x00;
	}
	else
	{
		while ((scancode & 0x0f) != 0) //取scancode的低四位，没变为全0，循环
		{
			line = scancode;                //行号
			XBYTE[0x8000] = scancode;     //给行赋扫描码，第一行为0x01
			if ((XBYTE[0x8000] & 0x0f) == col) //检测按键所在的行跳出循环
				break;
			scancode = scancode << 1;         //行扫描码左移一位，转下一行
		}
		col = col << 4;                     //把行码移到高四位
		keycode = col | line;
	}
	return keycode;
}


void timerAndInterruptInit()
{
	EA = 1;			// enable global interrupt

	TMOD = 0x11;	// set timer0 and timer1 to mode 1

	// (65535-X)*12/11.0592us = 10ms; X = 56319;
	TH0 = 0xDB;		// load 56319D to timer0
	TL0 = 0xFF;
	TR0 = 1;		// start timer0
	ET0 = 1;		// enable timer0 interrupt

	// (65535-X)*12/11.0592us=2ms; X=63691；  
	//TH1 = 0xF8;		// load 63691 to timer1 ，2ms
	//TL1 = 0xCB;
	//TR1 = 1;		// start timer0
	//ET1 = 1;		// enable timer0 interrupt
}


void setTemperature(unsigned char keycode)
{
	switch (keycode)
	{
	case 0x14: //row 3 col 1
		G_setting_temperature = (G_setting_temperature == 10000) ? 10000 : G_setting_temperature + 100;
		break;
	case 0x24:
		G_setting_temperature = (G_setting_temperature == 0) ? 0 : G_setting_temperature - 100;
		break;
	case 0x44:
		G_setting_temperature = (G_setting_temperature  >= 9000) ? 10000 : G_setting_temperature + 1000;
		break;
	case 0x84:
		G_setting_temperature = (G_setting_temperature  <= 1000) ? 0 : G_setting_temperature - 1000;
		break;
	default:
		break;
	}
	refreshLCDTable(0x01);
	LCDRefresh();
}


void refreshLCDTable(unsigned char mode)
{
	unsigned int temp = 0;
	if (mode & 0x01) {
		temp = G_setting_temperature;
		// refresh setting temperature
		temp /= 10;
		G_LCD_table[0][13] = temp % 10 + 0x30;
		temp /= 10;
		G_LCD_table[0][11] = temp % 10 + 0x30;
		temp /= 10;
		G_LCD_table[0][10] = temp % 10 + 0x30;
		temp /= 10;
		G_LCD_table[0][9] = 0 == temp ? 0x20: '1';
	}
	if (mode & 0x02) {
		// refresh measured temperature
		temp = G_measured_temperature;
		temp /= 10;
		G_LCD_table[1][13] = temp % 10 + 0x30;
		temp /= 10;
		G_LCD_table[1][11] = temp % 10 + 0x30;
		temp /= 10;
		G_LCD_table[1][10] = temp % 10 + 0x30;
		temp /= 10;
		G_LCD_table[1][9] = 0 == temp ? 0x20: '1';
	}
}


void PIDInit()
{
	G_pid_sum = 0;
	G_pid_last_temperature_difference = 0;
	G_pid_difference = 0;
}


void PIDupdate() 
{
	int temperature_difference = 0;
	temperature_difference = G_setting_temperature - G_measured_temperature;
	G_pid_sum += temperature_difference;
	G_pid_difference = temperature_difference - G_pid_last_temperature_difference;
	G_pid_last_temperature_difference = temperature_difference;
}


unsigned char getDutyCycle() 
{	
	//todo this function could be update.
	float duty_cycle = 0;
	PIDupdate();
	duty_cycle = PID_K_P * G_pid_last_temperature_difference + PID_K_I * G_pid_sum + PID_K_D * G_pid_difference;
	//duty_cycle = 0.001*G_pid_last_temperature_difference + PID_K_I * G_pid_sum + PID_K_D * G_pid_difference;
	//duty_cycle = G_pid_sum;

	if (duty_cycle < 0) {
		return G_duty_cycle = 0;
	}
	return  (G_duty_cycle = (unsigned int)duty_cycle);
	
	
	if (duty_cycle > 100) {
		return G_duty_cycle = 100;
	}
	else {
		if (duty_cycle < 0) {
			return G_duty_cycle = 0;
		}
		else {
			return G_duty_cycle = (unsigned int)duty_cycle;
		}
	}
}


void PWMoutput()
{
	if (G_timer0_count < G_duty_cycle) {
		pin_PWM_out = 1;
	}
	else {
		pin_PWM_out = 0;
	}
}


void timer0() interrupt 1 {
	TH0 = 0xDB;		// load 56319D to timer0, 10ms.
	TL0 = 0xFF;
	G_timer0_count++;
	G_timer0_flag = 1;
}