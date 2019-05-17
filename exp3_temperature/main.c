#include<REG51.h>
#include<absacc.h>
#define DELAY_TIMES 1

void LCDWriteControl(unsigned char control_word);
void LCDWriteData(unsigned char data_word);
void LCDWriteString(unsigned char * str_ptr, unsigned short length);
void LCDInit();
void delay(unsigned char times);


void main() 
{
	int i = 0;
	LCDInit();
	unsigned char init_table[7] = { "Initializing..." };
	LCDWriteString(init_table);
	while (1) {
		//todo
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
	int i = 0;
	for (i = 0; i < length; i++) {
		LCDWriteData(str_ptr[i]);
	}
}


void LCDInit() 
{
	LCDWriteControl(0x38); // 16*2显示，5*7点阵，8位数据口
	LCDWriteControl(0x0E); // 设置开启显示，不显示光标
	LCDWriteControl(0x06); // 写一个字符后地址指针+1
	LCDWriteControl(0x01); // 显示清零，数据指针清零0
	LCDWriteControl(0x80 + 0x01); // 设置数据地址指针从第一个开始
}