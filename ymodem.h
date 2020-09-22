//http://code.google.com/p/xtreamerdev/source/browse/trunk/rtdsr/?r=2#rtdsr%253Fstate%253Dclosed

/* ymodem for RTD Serial Recovery (rtdsr)
 *
 * copyright (c) 2011 Pete B. <xtreamerdev@gmail.com>
 *
 * based on ymodem.h for bootldr, copyright (c) 2001 John G Dorsey
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _HGL_YMODEM_H
#define _HGL_YMODEM_H

#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER           (3)     /* start, block, block-complement */
#define PACKET_TRAILER          (2)     /* CRC bytes */
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)
#define PACKET_TIMEOUT          (1)

#define FILE_NAME_LENGTH (64)
#define FILE_SIZE_LENGTH (16)


/* Number of consecutive receive errors before giving up: */
#define MAX_ERRORS    (5)

#define YMODEM_END_SESSION_SEND_CAN_NUM  0x07

#define YMODEM_TIMEOUT		60

/* ASCII control codes: */
enum ymodem_code {
    YMODEM_CODE_NONE = 0x0,
    YMODEM_CODE_SOH = 0x01,     /* start of 128-byte data packet */
    YMODEM_CODE_STX = 0x02,     /* start of 1024-byte data packet */
    YMODEM_CODE_EOT = 0x04,     /* end of transmission */
    YMODEM_CODE_ACK = 0x06,     /* receive OK */
    YMODEM_CODE_NAK = 0x015,    /* receiver error; retry */
    YMODEM_CODE_CAN = 0x018,    /* two of these in succession aborts transfer */
    YMODEM_CODE_CRC = 0x43,     /* use in place of first NAK for CRC mode */
};

enum ymodem_stage {
    YMODEM_STAGE_NONE,
    /* set when C is send */
    YMODEM_STAGE_ESTABLISHING,
    /* set when we've got the packet 0 and sent ACK and second C */
    YMODEM_STAGE_ESTABLISHED,
    /* set when the sender respond to our second C and recviever got a real
     * data packet. */
    YMODEM_STAGE_TRANSMITTING,
    /* set when the sender send a EOT */
    YMODEM_STAGE_FINISHING,
    /* set when transmission is really finished, i.e., after the NAK, C, final
     * NULL packet stuff. */
    YMODEM_STAGE_FINISHED,
};

struct ymodem {
    char file_name[FILE_NAME_LENGTH];
    unsigned long file_size;
    unsigned char *data;    //If it is sending mode, it is file data, if it is receiving file, it will not be processed. 如果是发送文件,为文件数据,如果是接收文件,则不处理
    enum ymodem_stage stage;

    int (*putchar)(unsigned char ch);
    int (*getchar)(void);
    int (*begin_packet_cb)(const unsigned char *buf, unsigned long len);
    int (*data_packet_cb)(const unsigned char *buf, unsigned long len);
    int (*end_packet_cb)(const unsigned char *buf, unsigned long len);

};


int ymodem_receive(struct ymodem *ymodem);

int ymodem_send(struct ymodem *ymodem);

#endif /*_HGL_YMODEM_H*/