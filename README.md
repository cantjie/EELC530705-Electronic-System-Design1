# 单片机实验之万年历与数字钟

* [markdown语法](https://www.jianshu.com/p/191d1e21f7ed )
* [markdown在线浏览器1](https://jbt.github.io/markdown-editor/ )
* [markdown在线浏览器2(both support table, choose a faster one)](https://dillinger.io/ )

# 任务要求
```
	要求：
	在实验板上编程实现2019年日历和实时时钟： 
	1.时-分-秒（2位-2位-2位）显示 
	2.可通过按键置入时间值(参照电子表设置时间工作模式）
	3.可通过按键控制在LED上从右向左滚动显示年_月_ 日3次，如：2013_01_20空空2013_01_20 
	4.实现每日闹铃提醒功能，闹铃时间可用按键设置。闹铃采用提示音表示。 
	5.实现秒表功能。 
	6.实现定时器功能（预置定时时间，按键启动，倒计时，计到0响提示音。 
	7.设计实现音乐提示音。 
```
# 用户手册、使用说明
```php
//todo
```

# License
```
	禁止将代码的整体或部分用于商业用途。
	将代码的整体或部分直接用于教育用途请联系作者(QQ46472001,Email:cantjie@163.com)获得授权。
```


# 开发日志
	
## 2019-01-11
1. 花了一上午时间寻找为什么秒到60的时候不进位。全速运行的时候不进位，但是断点单步运行调试就正常。
	* 发现问题为项目设置里面开启了serial interrupt
2. 花了半个下午的时间寻找为什么按键一下会跳好多数，找到原因发现是把keycode清零的位置不对。



## 2019-02-03
1. 先前已经完成了前三项任务。现在从第四项开始，增加闹钟功能。

## 2019-02-04
1. 播放音乐功能采用两个计时器配合。timer0用来计1/4拍的长度，此处定为150ms。timer1用来处理对应频率。
2.简谱音符->频率->计数器初值的两次对应，可以在程序外自己算好，直接将计数器初值放入数组，也可以把简谱音符写到程序里面，在程序里面进行转换，此处采用第二种方法。虽然可能增加了系统开销，但是增强了程序的可读性。

## 2019-02-05
1. [data segment too large](https://bbs.csdn.net/topics/30192341)
2. timer0一直保持50ms不变，而timer1不播放音乐的时候是2ms，500Hz，播放音乐时，频率一般比500Hz高，因此显示效果反而更好，更亮。但日期显示模式下会使滚动加快，设置模式下会使闪烁加快。
但无论在什么模式下，闹钟都会发挥作用，不会因为处于设置模式而错过闹钟。
```
	(65535 - X) * 12 / 11.0592us = yms;   
	65535-X = y*11.0592/12 *1000
	X = 65535 - y*11059.2/12  
	yms = 1000/freq
	 X = 65535- 11059200/12/freq
	pay attention to the freq here, it should be double of the frequency.
	at half of T, we'd change the pin voltage.
	0xF9,0xF9,0xFA,0xFA,0xFB,0xFB,0xFC
	0x20,0xDF,0x8A,0xD6,0x67,0xE7,0x5A

	0xFC,0xFC,0xFD,0xFD,0xFD,0xFD,0xFE
	0x8D,0xED,0x43,0x6A,0xB3,0xF3,0x2C

	0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF
	0x46,0x76,0xA1,0xB5,0xD9,0xF9,0x14

```
3. 采用了简谱音符->计数器初值的对应方法。
4. 着手秒表功能。秒表误差分析
	```
	为了使用timer1 的同时不影响显示功能。y=2ms无法改变。
	这样就会因为X不是整数而造成误差。
	若要将y设置成5ms,10ms,虽然X是整数，但使LED显示频率过低。
	X = 65535 - y*11059.2/12  
	y = 2ms
	X = 63691.8  
	65535 - X = 1843.2 
	0.2/1843.2*500*60=3.25520833333s
	每60s 差 3.255s
	```
5. 秒表需求和功能分析
	```
	进入秒表功能后有两个按键可用，需要实现:
		开始计时，暂停显示，恢复显示，停止计时，清零。
	三个状态：
		闲置状态（停止状态），计时状态，暂停状态。
	```

	起始状态 | 按键功能(对应按键) | 到达状态 | 备注
	:--:|:--:|:--:|:--:
	闲置 | 开始计时(L) | 计时 | 从当前值开始计时
	闲置 | 清零\(R\) | 闲置 | 恢复到0
	计时 | 暂停显示(L)  | 暂停 | 显示值固定，但后台继续计时
	计时 | 停止计时\(R\) | 闲置 | 显示值固定，后台也停止计时
	暂停 | 暂停显示(L) | 暂停 | 显示值恢复到后台计时值，但固定于此值，类似于读完第一名成绩后，接着直接读第二名成绩
	暂停 | 恢复显示\(R\) | 计时 | 显示值恢复到后台计时值，并实时显示计时值
	
	```
	如此，每个状态下只有两个按键功能，可以用两个按键实现。
	使用第三行中间两个按键，规定按键功能也附到上表中。L/R表示左/右。
	```
	
## 2019-02-06
1. 继续完成秒表功能。三个状态已经完成，主要剩余五个按键功能。秒表功能完成。
2. 定时器需求和功能分析
	```
	有两个按键可用，需要实现定时器设置和开启，采用如下显示方式：
	X-MMM-SS
		X用来控制计时开关和选歌。0表示停止，1~9用来选歌；
		MMM表示分钟，最高支持250分钟；
		SS表示秒，范围0~59。
	两个按键L和R，具有如下功能：
		L切换设置位，
		R对设置位自增。
	有2个状态：
		设置，计时（应支持后台计时）
		当X不为0，即进入计时状态。
	```
3. 定时器误差分析
	```
	由于需要和时钟使用同一个timer0_count,
	而在开始计时时，timer0_count不一定为0，
	因此可能会产生0~1秒的误差。
	可认为误差服从均匀分布E_sec~U(0,1)。
	```
4. 功能全部完成。若未发现新bug，下一步工作只需增加乐谱。

## 2019-02-07
1. 增加音乐。目前有两个铃声《世上只有妈妈好》《假如爱有天意》










