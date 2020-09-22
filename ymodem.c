#include "ymodem.h"
#include <stdio.h>
#include <string.h>

#define YMODEM_SEND_MAX_PACKET_SIZE      PACKET_1K_SIZE


#define YMODEM_DEBUG   0

#if YMODEM_DEBUG
#define ym_printf(...)      printf(__VA_ARGS__)
#else
#define ym_printf(...)
#endif /*YMODEM_DEBUG*/

/* http://www.ccsinfo.com/forum/viewtopic.php?t=24977 */
unsigned short crc16(const unsigned char *buf, unsigned long count)
{
    unsigned short crc = 0;
    int i;

    while(count--) {
        crc = crc ^ *buf++ << 8;

        for (i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = crc << 1 ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

static unsigned long str_to_u32(char* str)
{
    const char *s = str;
    unsigned long acc;
    int c;

    /* strip leading spaces if any */
    do {
        c = *s++;
    } while (c == ' ');

    for (acc = 0; (c >= '0') && (c <= '9'); c = *s++) {
        c -= '0';
        acc *= 10;
        acc += c;
    }
    return acc;
}

static const char *u32_to_str(unsigned int val)
{
        /* Maximum number of decimal digits in u32 is 10 */
        static char num_str[11];
        int  pos = 10;
        num_str[10] = 0;

        if (val == 0) {
                /* If already zero then just return zero */
                return "0";
        }

        while ((val != 0) && (pos > 0)) {
                num_str[--pos] = (val % 10) + '0';
                val /= 10;
        }

        return &num_str[pos];
}

static int ymodem_show_packet(unsigned char *packet, unsigned int packet_size)
{
#if YMODEM_DEBUG    
    switch (packet[0]) {
    case YMODEM_CODE_SOH:
        ym_printf("SOH ");
        break;
    case YMODEM_CODE_STX:
        ym_printf("STX ");
        break;
    case YMODEM_CODE_EOT:
        ym_printf("EOT ");
        break;
    case YMODEM_CODE_ACK:
        ym_printf("ACK ");
        break;
    case YMODEM_CODE_NAK:
        ym_printf("NAK ");
        break;
    case YMODEM_CODE_CAN:
        ym_printf("CAN ");
        break;
    default:
        break;
    }
    if (packet_size) {
        ym_printf("%02X %02X Data[%d] CRC CRC\r\n", packet[1], packet[2], packet_size);
    } else {
        ym_printf("\r\n");
    }
#endif
	return 0;
}

static int ymodem_putc_and_show(struct ymodem *ymodem, unsigned char ch)
{
    if (ch == YMODEM_CODE_EOT) {
        ym_printf("EOT\r\n");
        return ymodem->putchar(ch);
    }

#if YMODEM_DEBUG
    ym_printf("\t\t\t\t");
    
    switch (ch) {
    case YMODEM_CODE_CRC:
        ym_printf("C\r\n");
        break;
    case YMODEM_CODE_CAN:
        ym_printf("CAN\r\n");
        break;
    case YMODEM_CODE_NAK:
        ym_printf("NAK\r\n");
        break;
    case YMODEM_CODE_ACK:
        ym_printf("ACK\r\n");
        break;
    default:
        break;
    }
#endif  /*YMODEM_DEBUG*/
    return ymodem->putchar(ch);
}

static int ymodem_getc_and_show(struct ymodem *ymodem)
{
    int ch;

    ch = ymodem->getchar();

#if YMODEM_DEBUG
    ym_printf("\t\t\t\t");
    
    switch (ch) {
    case YMODEM_CODE_CRC:
        ym_printf("C\r\n");
        break;
    case YMODEM_CODE_CAN:
        ym_printf("CAN\r\n");
        break;
    case YMODEM_CODE_NAK:
        ym_printf("NAK\r\n");
        break;
    case YMODEM_CODE_ACK:
        ym_printf("ACK\r\n");
        break;
    default:
        break;
    }
#endif /*YMODEM_DEBUG*/
    return ch;
}

static int receive_packet(struct ymodem *ymodem, unsigned char *data, unsigned long *length)
{
    int i, c;
    unsigned long packet_size;

    *length = 0;

    c = ymodem->getchar();
    if (c < 0)
        return -1;
    
    *data = (unsigned char)c;

    switch(c) {
    case YMODEM_CODE_SOH:
            packet_size = PACKET_SIZE;
            break;
    case YMODEM_CODE_STX:
            packet_size = PACKET_1K_SIZE;
            break;
    case YMODEM_CODE_EOT:
            packet_size = 0;
            goto OUT;
    case YMODEM_CODE_CAN:
            c = ymodem->getchar();
            if (c == YMODEM_CODE_CAN) {
                    return 0;
            }
    default:
            /* This case could be the result of corruption on the first octet
            * of the packet, but it's more likely that it's the user banging
            * on the terminal trying to abort a transfer. Technically, the
            * former case deserves a NAK, but for now we'll just treat this
            * as an abort case.
            */
            return -1;
    }

    

    for(i = 1; i < (packet_size + PACKET_OVERHEAD); ++i) {
            c = ymodem->getchar();
            if (c < 0) {
                return -1;
            }
            data[i] = (unsigned char)c;
    }

    /* Just a sanity check on the sequence number/complement value.
        * Caller should check for in-order arrival.
        */
    if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff)) {
        return 1;
    }

    if (crc16(data + PACKET_HEADER, packet_size + PACKET_TRAILER) != 0) {
        return 1;
    }
    *length = packet_size;
OUT:
    ymodem_show_packet(data, packet_size);

    return 0;
}

                                               
static int ymodem_handshake(struct ymodem *ymodem)
{
    int timeout = YMODEM_TIMEOUT;
    int ret = -1;
    unsigned long packet_size = 0;
    unsigned char packet[PACKET_OVERHEAD+PACKET_1K_SIZE] = {0};
    char *file_size;

    ymodem->stage = YMODEM_STAGE_ESTABLISHING;

    while (timeout--) {
        ymodem_putc_and_show(ymodem, YMODEM_CODE_CRC);  //send 'C'

        ret = receive_packet(ymodem, packet, &packet_size);
        if (ret == 0 && packet_size >= PACKET_SIZE) {
            
            strcpy(ymodem->file_name, (const char *)&packet[PACKET_HEADER]);
            file_size = (char *)&packet[PACKET_HEADER + strlen(ymodem->file_name) + 1];
            ymodem->file_size = str_to_u32(file_size);
            ym_printf("file name : %s, size : %ld\r\n", ymodem->file_name, ymodem->file_size);
            if (ymodem->begin_packet_cb) {
                if (ymodem->begin_packet_cb(&packet[PACKET_HEADER], packet_size) != 0)
                    return -1;
            }
            ymodem_putc_and_show(ymodem, YMODEM_CODE_ACK);
            ymodem_putc_and_show(ymodem, YMODEM_CODE_CRC);
            break;
        } else if (ret == -1) {
            //timeout
        } else if (ret == 1) {
            //check failed
        }
    }
    if (timeout <= 0)
        return -1;

    return 0;
}

int ymodem_receive_file_data(struct ymodem *ymodem)
{
    unsigned char packet[PACKET_OVERHEAD+PACKET_1K_SIZE];
    unsigned long packet_size = 0;
    int i, ret = -1;
    int retry = 0;

    ymodem->stage = YMODEM_STAGE_TRANSMITTING;

    while (1) {
        ret = receive_packet(ymodem, packet, &packet_size);
        if (ret == 0) {
            if (packet_size) {
                if (ymodem->data_packet_cb &&
                        ymodem->data_packet_cb(&packet[PACKET_HEADER], packet_size) != 0) {
                    return -1;
                }
                ymodem_putc_and_show(ymodem, YMODEM_CODE_ACK);
            } else if (packet[0] == YMODEM_CODE_CAN) {
                /* the spec require multiple CAN */
                for (i = 0; i < YMODEM_END_SESSION_SEND_CAN_NUM; i++) {
                    ymodem_putc_and_show(ymodem, YMODEM_CODE_CAN);
                }
            } else if (packet[0] == YMODEM_CODE_EOT) {
                //EOT
                return 0;
            }
        } else {
            if (retry++ > MAX_ERRORS) {
                break;
            } else {
                ym_printf("retry!\r\n");
            }
        }
    }
    return -1;
}

static int ymodem_recv_finsh(struct ymodem *ymodem)
{
    int ch, ret;
    unsigned char packet[PACKET_OVERHEAD+PACKET_1K_SIZE];
    unsigned long packet_size = 0;

    ymodem->stage = YMODEM_STAGE_FINISHING;
    /* we already got one EOT in the caller. invoke the callback if there is
     * one. */
    ymodem_putc_and_show(ymodem, YMODEM_CODE_NAK);

    ch = ymodem->getchar();
    if (ch != YMODEM_CODE_EOT)
        return -1;
    ymodem_show_packet((unsigned char*)&ch, 0);

    ymodem_putc_and_show(ymodem, YMODEM_CODE_ACK);
    ymodem_putc_and_show(ymodem, YMODEM_CODE_CRC);

    ret = receive_packet(ymodem, packet, &packet_size);
    if (ret != 0)
        return -1;

    if (packet_size != PACKET_SIZE)
        return -1;

    ymodem_putc_and_show(ymodem, YMODEM_CODE_ACK);

    if (ymodem->end_packet_cb)
            ymodem->end_packet_cb(&packet[PACKET_HEADER], packet_size);

    if (packet[3] != 0x0) { //还有文件要接收
        if (ymodem->begin_packet_cb)
            ymodem->begin_packet_cb(&packet[PACKET_HEADER], packet_size);
        ymodem_putc_and_show(ymodem, YMODEM_CODE_CRC);
        return 0;
    }
    
    ymodem->stage = YMODEM_STAGE_FINISHED;
    
    return 0;
}
/*
 Figure 3.  YMODEM Batch Transmission Session (1 file)
SENDER                                  RECEIVER
                                        "sb foo.*<CR>"
"sending in batch mode etc."
                                        C (command:rb)
SOH 00 FF foo.c NUL[123] CRC CRC
                                        ACK
                                        C
SOH 01 FE Data[128] CRC CRC
                                        ACK
SOH 02 FC Data[128] CRC CRC
                                        ACK
SOH 03 FB Data[100] CPMEOF[28] CRC CRC
                                        ACK
EOT
                                        NAK
EOT
                                        ACK
                                        C
SOH 00 FF NUL[128] CRC CRC
                                        ACK
*/

/*
Figure 4.  YMODEM Batch Transmission Session (2 files)
SENDER                                  RECEIVER
                                        "sb foo.c baz.c<CR>"
"sending in batch mode etc."
                                        C (command:rb)
SOH 00 FF foo.c NUL[123] CRC CRC
                                        ACK
                                        C
SOH 01 FE Data[128] CRC CRC
                                        ACK
SOH 02 FC Data[128] CRC CRC
                                        ACK
SOH 03 FB Data[100] CPMEOF[28] CRC CRC
                                        ACK
EOT
                                        NAK
EOT
                                        ACK
                                        C
SOH 00 FF baz.c NUL[123] CRC CRC
                                        ACK
                                        C
SOH 01 FB Data[100] CPMEOF[28] CRC CRC
                                        ACK
EOT
                                        NAK
EOT
                                        ACK
                                        C
SOH 00 FF NUL[128] CRC CRC
                                        ACK
*/
int ymodem_receive(struct ymodem *ymodem)
{
    int ret = -1;

    if (NULL == ymodem || NULL == ymodem->getchar || NULL == ymodem->putchar)
        return -1;

    ymodem->stage = YMODEM_STAGE_NONE;

    //handshake
    ret = ymodem_handshake(ymodem);
    if (ret != 0)
        return ret;
    
    while (1) {
        ret = ymodem_receive_file_data(ymodem);
        if (ret != 0)
            return ret;

        ret = ymodem_recv_finsh(ymodem);
        if (ret != 0)
            return ret;
        if (ymodem->stage == YMODEM_STAGE_FINISHED)
            break;
    }

    return 0;
}


static void send_packet(struct ymodem *ymodem, unsigned char *data, int block_no, int packet_size)
{
    int count, crc;
    unsigned char packet[3];

    crc = crc16(data, packet_size);
    /* 128 byte packets use SOH, 1K use STX */
    
    packet[0] = (packet_size==PACKET_SIZE)?YMODEM_CODE_SOH:YMODEM_CODE_STX;
    ymodem->putchar(packet[0]);

    packet[1] = (block_no & 0xFF);
    ymodem->putchar(packet[1]);

    packet[2] = (~block_no & 0xFF);
    ymodem->putchar(packet[2]);

    for (count=0; count<packet_size; count++)
        ymodem->putchar(data[count]);

    ymodem->putchar((crc >> 8) & 0xFF);
    ymodem->putchar(crc & 0xFF);

    ymodem_show_packet(packet, packet_size);
}


/* Send block 0 (the filename block). filename might be truncated to fit. */
static void send_packet0(struct ymodem *ymodem, char* filename, unsigned long size)
{
    unsigned long count = 0;
    unsigned char block[PACKET_SIZE];
    const char* num;

    if (filename) {
        while (*filename && (count < PACKET_SIZE-FILE_SIZE_LENGTH-2)) {
            block[count++] = *filename++;
        }
        block[count++] = 0;

        num = u32_to_str(size);
        while(*num) {
            block[count++] = *num++;
        }
        ym_printf("file name: %s, size: %d\r\n", block, size);
    }

    while (count < PACKET_SIZE) {
        block[count++] = 0;
    }

    send_packet(ymodem, block, 0, PACKET_SIZE);
    if (ymodem->begin_packet_cb)
        ymodem->begin_packet_cb(block, PACKET_SIZE);
}

static void send_data_packets(struct ymodem *ymodem)
{
    int blockno = 1;
    unsigned long i, size, send_size;
    int ch;
    unsigned char packet_data[PACKET_1K_SIZE];
    unsigned char *data = ymodem->data;
    int retry = 0;

    size = ymodem->file_size;
    while (size > 0) {
            if (size > PACKET_SIZE) {
                send_size = PACKET_1K_SIZE;
            } else {
                send_size = PACKET_SIZE;
            }
            if (send_size > YMODEM_SEND_MAX_PACKET_SIZE)
                send_size = YMODEM_SEND_MAX_PACKET_SIZE;
                
            if (size < send_size) {
                memcpy(packet_data, data, size);
                //需要填充0x1A
                for (i = size; i < send_size; i++)
                    packet_data[i] = 0x1A;
            } else {
                memcpy(packet_data, data, send_size);
            }

            send_packet(ymodem, packet_data, blockno, send_size);

            if (ymodem->data_packet_cb)
                ymodem->data_packet_cb(packet_data, send_size);

            ch = ymodem_getc_and_show(ymodem);
            if (ch == YMODEM_CODE_ACK) {
                retry = 0;
                blockno++;
                data += send_size;
                if (size < send_size)
                    size = 0;
                else
                    size -= send_size;
            } else {
                if (ch == YMODEM_CODE_NAK) {
                    if (retry++ > MAX_ERRORS)
                        return;
                }
                if((ch == YMODEM_CODE_CAN) || (ch == -1)) {
                    return;
                }
            }
    }

    do {
        ymodem_putc_and_show(ymodem, YMODEM_CODE_EOT);
        ch = ymodem_getc_and_show(ymodem);
    } while((ch != YMODEM_CODE_ACK) && (ch != -1));

    /* Send last data packet */
    if (ch == YMODEM_CODE_ACK) {
        ch = ymodem_getc_and_show(ymodem);
        if (ch == YMODEM_CODE_CRC) {
            do {
                send_packet0(ymodem, 0, 0);
                ch = ymodem_getc_and_show(ymodem);
            } while((ch != YMODEM_CODE_ACK) && (ch != -1));
        }
    }
    if (ymodem->end_packet_cb)
        ymodem->end_packet_cb(NULL, 0);
}

/*
 * 为了简单,需要一次性将文件内容复制到ymodem->data中,
 * 大多数情况都是下位机发送,不会遇到内存问题.
 * 
 */
int ymodem_send(struct ymodem *ymodem)
{
    int ch, crc_nak = 1;
    int retry = 0;

    do {
        ch = ymodem_getc_and_show(ymodem);
    } while (retry++ < YMODEM_TIMEOUT && ch != YMODEM_CODE_CRC);

    if (ch == YMODEM_CODE_CRC) {
        while (1) {
            send_packet0(ymodem, ymodem->file_name, ymodem->file_size);
            ch = ymodem_getc_and_show(ymodem);
            if (ch == YMODEM_CODE_ACK) {
                ch = ymodem_getc_and_show(ymodem);
                if (ch == YMODEM_CODE_CRC) {
                    send_data_packets(ymodem);
                    return 0;
                }
            } else if ((ch == YMODEM_CODE_CRC) && (crc_nak)) {
                crc_nak = 0;
                continue;
            } else if ((ch != YMODEM_CODE_NAK) || (crc_nak)) {
                break;
            }
        }
    } else {
        ymodem_putc_and_show(ymodem, YMODEM_CODE_CAN);
        ymodem_putc_and_show(ymodem, YMODEM_CODE_CAN);
    }

    return -1;
}