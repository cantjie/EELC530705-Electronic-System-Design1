# 简易温度测控系统设计

## 单片机LCD部分
1. AJ02为低电平时（0xAxxx)，1602的使能变高，可以写数据。
2. XBYTE[0xA000] 写指令; XBYTE[0xA001] 读忙标志或地址;XBYTE[0xA002] 写数据; XBYTE[0xA003] 读。