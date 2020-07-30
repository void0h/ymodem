# ymodem
linux c ymodem

### 在调用receive/send时,需要先设置好struct ymodem的几个回调函数.如ymodem_test中.

### help
```
/userdata/test # ./ymodem -h
Usage : ./ymodem [options]
options:
eg send Ymodem: ./ymodem -d /dev/ttyUSB0 -s /sdcard/test.txt
eg receive Ymodem: ./ymodem -d /dev/ttyUSB0
	 -d <device name>		device path.设置设备名,默认/dev/ttyUSB0.
	 -s <file name>       send Ymodem, receiving mode if not set.发送文件,如果不选,默认为接收文件
	 -t <timeout>         Set getchar timeout,def:10(1s), 1=100ms.设置getchar超时,1为100毫秒,默认10=>1s.
	 --help			display specific types of command line options.
```

### recevice file
```
/userdata/test # ./ymodem -d /dev/ttyUSB0 
dev path: /dev/ttyUSB0
open dev: /dev/ttyUSB0
Receice Ymodem!
				C
				C
				C
SOH 00 FF Data[128] CRC CRC
				ACK
				C
file name : xymodem.pdf, size : 53245
STX 01 FE Data[1024] CRC CRC
				ACK
STX 02 FD Data[1024] CRC CRC
				ACK
.
.
.
STX 33 CC Data[1024] CRC CRC
				ACK
STX 34 CB Data[1024] CRC CRC
				ACK
EOT 
				NAK
EOT 
				ACK
				C
SOH 00 FF Data[128] CRC CRC
				ACK
Recevice complete!
file: xymodem.pdf, size: 53245
/userdata/test # 
```

### send file
```
/userdata/test # 
/userdata/test # ./ymodem -d /dev/ttyUSB0  -s xymodem.pdf 
dev path: /dev/ttyUSB0
send file: xymodem.pdf.
open dev: /dev/ttyUSB0
Send Ymodem! file: xymodem.pdf
				CAN
				CAN
				C
file name: xymodem.pdf, size: 53245
SOH 00 FF Data[128] CRC CRC
				ACK
				C
STX 01 FE Data[1024] CRC CRC
				ACK
STX 02 FD Data[1024] CRC CRC
				ACK
.
.
.
STX 33 CC Data[1024] CRC CRC
				ACK
STX 34 CB Data[1024] CRC CRC
				ACK
EOT
				NAK
EOT
				ACK
				C
SOH 00 FF Data[128] CRC CRC
				ACK
Transfer complete!
```