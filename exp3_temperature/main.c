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
	LCDWriteControl(0x38); // 16*2��ʾ��5*7����8λ���ݿ�
	LCDWriteControl(0x0E); // ���ÿ�����ʾ������ʾ���
	LCDWriteControl(0x06); // дһ���ַ����ַָ��+1
	LCDWriteControl(0x01); // ��ʾ���㣬����ָ������0
	LCDWriteControl(0x80 + 0x01); // �������ݵ�ַָ��ӵ�һ����ʼ
}