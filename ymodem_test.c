#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#include <termios.h>

#include <getopt.h>

#include "ymodem.h"
#include "serial_port.h"


#define DEV_PATH_DEF    "/dev/ttyUSB0"
#define BAUDRATE_DEF    115200
#define TIMEOUT_DEF     20  //10*100ms




static int g_fd;
static struct ymodem ymodem;
static FILE* g_fp;

int _putchar(unsigned char ch)
{
    return write(g_fd, &ch, 1);
}

int _getchar(void)
{
    ssize_t size;
    unsigned char ch;

    size = read(g_fd, &ch, 1);
    if (size == 1)
        return ch;
    else
        return -1;
}

int recv_file_size = 0;

int begin_packet_callback(const unsigned char *buf, unsigned long len)
{
    recv_file_size = 0;

    g_fp = fopen(ymodem.file_name,"wb+");
	if(g_fp == NULL) {
		printf("open %s error\r\n", ymodem.file_name);
		return -1;
	}

    return 0;
}

int data_packet_callback(const unsigned char *buf, unsigned long len)
{
    int invalid = 0;

    //由于我传入data_packet_cb(&packet[PACKET_HEADER], packet_size)的
    //是&packet[PACKET_HEADER], 是不带index的, 如果需要, 可以将整个包传入(ymodem.c line:273),
    //再在这里判断index,避免出现重复写入或丢数据

    recv_file_size += len;

    if (recv_file_size > ymodem.file_size) {
        invalid = recv_file_size - ymodem.file_size;     
    }
    fwrite(buf, 1, len - invalid, g_fp);
    return 0;
}

int end_packet_callback(const unsigned char *buf, unsigned long len)
{
    fclose(g_fp);
    return 0;
}

int recevice(void)
{
    int ret;
    
    printf("Receice Ymodem!\r\n");

    ret =  ymodem_receive(&ymodem);
    
    //需要再发送两个0x4F,xshell/secureCRT才会提示传送完成
    sleep(1);   
    ymodem.putchar(0x4F);
    ymodem.putchar(0x4F);
    
    return ret;
}


int send_begin_packet_callback(const unsigned char *buf, unsigned long len)
{
    return 0;
}

int send_data_packet_callback(const unsigned char *buf, unsigned long len)
{
    return 0;
}

int send_end_packet_callback(const unsigned char *buf, unsigned long len)
{
    return 0;
}

int send(const char* file_name)
{
    int ret, fd, index, size;

    printf("Send Ymodem! file: %s\r\n", file_name);

    strcpy(ymodem.file_name, file_name);
    ymodem.data = (unsigned char *)malloc(300*1024);

    fd = open(file_name, O_RDONLY);
    if (fd <= 0)
        return -1;
    //get file data
    index = 0;
    do {
        size = read(fd, &ymodem.data[index], 1024);
        if (size)
            index += size;
    } while(size > 0);
    close(fd);
    ymodem.file_size = index;
    ret = ymodem_send(&ymodem);

    free(ymodem.data);
    
    return ret;
}

void ymodem_help(void)
{
    printf("Usage : ./ymodem [options]\n");
    printf("options:\r\n");
    printf("eg send Ymodem: ./ymodem -d /dev/ttyUSB0 -s /sdcard/test.txt\r\n");
    printf("eg receive Ymodem: ./ymodem -d /dev/ttyUSB0\r\n");
    printf("\t -d <device name>		device path.设置设备名,默认/dev/ttyUSB0.\r\n");
    printf("\t -s <file name>       send Ymodem, receiving mode if not set.发送文件,如果不选,默认为接收文件\r\n");
    printf("\t -t <timeout>         Set getchar timeout,def:10(1s), 1=100ms.设置getchar超时,1为100毫秒,默认10=>1s.\r\n");
    printf("\t --help			display specific types of command line options.\r\n\r\n");
}

int main(int argc, char *argv[])
{
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    int c = -1;
    int timeout = TIMEOUT_DEF;
    char dev_path[128] = DEV_PATH_DEF;
    int is_send = 0;
    char g_file_name[256]  = {0};
    int putchar_flag = 0;
    int putchar_value = 0;

    while (1) {
        struct option long_options[] = {
            {"help",    no_argument, 0,  'h' },
            {0,         0,             0,  0 }
        };

        c = getopt_long_only(argc, argv, "c:s:d:t:h",
				long_options, &option_index);
		if (c == -1)
			break;

        switch (c) {
        case 0:
            break;
        case 'c':
            putchar_flag = 1;
            putchar_value = optarg[0];
            printf("putchar value: %c\r\n", putchar_value);
            break;
        case 'd':
            strcpy(dev_path, optarg);
            printf("dev path: %s\r\n", dev_path);
            break;
        case 's':
            strcpy(g_file_name, optarg);
            printf("send file: %s.\r\n", g_file_name);
            is_send = 1;
            break;
        case 't':
            timeout = atoi(optarg);
            printf("timeout: %d*100ms.\r\n", timeout);
            break;
        case 'h':
        case '?':
            ymodem_help();
            return 0;
            // break;

        default:
            break;
        }    

    }

    if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");

        ymodem_help();
		return 0;
	}

    //init serial
    printf("open dev: %s\r\n", dev_path);
    g_fd = open(dev_path, O_RDWR);
    if (g_fd <= 0) {
        printf("open %s failed!\r\n", dev_path);
        return -1;
    }
    serial_set(g_fd, BAUDRATE_DEF, 8, 'N', 1, timeout, 0); //1vtime = 100ms, 10=1000ms=1s

    ymodem.putchar = _putchar;
    ymodem.getchar = _getchar;



    
    if (putchar_flag) {
        ymodem.putchar(putchar_value);
        printf("putchar: %c/%d\r\n", putchar_value,putchar_value);
        return 0;
    }

    if (is_send) {
        ymodem.begin_packet_cb = send_begin_packet_callback;
        ymodem.data_packet_cb = send_data_packet_callback;
        ymodem.end_packet_cb = send_end_packet_callback;

        if (send(g_file_name) == 0) {
            printf("Transfer complete!\r\n");
        } else {
            printf("Send Ymodem err!\r\n");
            printf("stage: %d\r\n", ymodem.stage);
        }
    } else {
        ymodem.begin_packet_cb = begin_packet_callback;
        ymodem.data_packet_cb = data_packet_callback;
        ymodem.end_packet_cb = end_packet_callback;

        if (recevice() == 0) {
            printf("Recevice complete!\r\n");
            printf("file: %s, size: %d\r\n", ymodem.file_name, ymodem.file_size);
        } else {
            printf("Recevice Ymodem err!\r\n");
            printf("stage: %d\r\n", ymodem.stage);
        }
    }

    close(g_fd);
    
    return 0;
}
