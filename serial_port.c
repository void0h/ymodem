//
// Created by hongg on 2020/3/30.
//

#include "serial_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <strings.h>


int serial_set(int fd, int speed, int bits, char event, int stop, int vtime, int vmin)
{
    struct termios new_tio;

    if (tcgetattr(fd, &new_tio) != 0)
        return -1;

    bzero(&new_tio, sizeof(new_tio));
    new_tio.c_cflag |= CLOCAL | CREAD;
    new_tio.c_cflag &= ~CSIZE;

    switch (bits) {
    case 7:
        new_tio.c_cflag |= CS7;
        break;
    case 8:
        new_tio.c_cflag |= CS8;
        break;
    default :
        new_tio.c_cflag |= CS8;
        break;
    }

    switch (event) {
    case 'o':
    case 'O':
        new_tio.c_cflag |= PARENB;
        new_tio.c_cflag |= PARODD;
        new_tio.c_iflag |= (INPCK|ISTRIP);
        break;
    case 'e':
    case 'E':
        new_tio.c_cflag |= (INPCK|ISTRIP);
        new_tio.c_cflag |= PARENB;
        new_tio.c_iflag &= ~PARODD;
        break;
    case 'n':
    case 'N':
        new_tio.c_cflag &= ~PARENB;
        break;
    default:
        new_tio.c_cflag &= ~PARENB;
        break;
    }

    switch (speed) {
    case 2400:
        cfsetispeed(&new_tio, B2400);
        cfsetospeed(&new_tio, B2400);
        break;
    case 4800:
        cfsetispeed(&new_tio, B4800);
        cfsetospeed(&new_tio, B4800);
        break;
    case 9600:
        cfsetispeed(&new_tio, B9600);
        cfsetospeed(&new_tio, B9600);
        break;
    case 19200:
        cfsetispeed(&new_tio, B19200);
        cfsetospeed(&new_tio, B19200);
        break;
    case 38400:
        cfsetispeed(&new_tio, B38400);
        cfsetospeed(&new_tio, B38400);
        break;
    case 57600:
        cfsetispeed(&new_tio, B57600);
        cfsetospeed(&new_tio, B57600);
        break;
    case 115200:
        cfsetispeed(&new_tio, B115200);
        cfsetospeed(&new_tio, B115200);
        break;
    default:
        cfsetispeed(&new_tio, B115200);
        cfsetospeed(&new_tio, B115200);
        break;
    }

    if (stop == 1)
        new_tio.c_cflag &= ~CSTOPB;
    else
        new_tio.c_cflag |= CSTOPB;

    new_tio.c_cc[VTIME] = vtime;
    new_tio.c_cc[VMIN] = vmin;

    if ((tcsetattr(fd, TCSANOW, &new_tio)) != 0)
        return -3;
    return 0;
}