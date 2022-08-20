#ifndef __SRRPX_H // simple request response protocol
#define __SRRPX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Request: >[seqno],[^|0|$],[lenth]:[header]{data}<crc8>\0
 *   >0,$,0061:/hello/x{name:'yon',age:'18',equip:['hat','shoes']}<crc8>\0
 *   >1,^,0013:/he<crc8>\0
 *   >2,0,0015:llo/y<crc8>\0
 *   >3,$,0052:{name:'myu',age:'12',equip:['gun','bomb']}<crc8>\0
 *
 * Response: <[seqno],[^|0|$],[lenth]:[request-crc8]{data}<crc8>\0
 *   <0,$,0062:0x13/hello/x{err:0,errmsg:'succ',data:{msg:'world'}}<crc8>\0
 *   <1,$,0061:0xcc/hello/y{err:1,errmsg:'fail',data:{msg:'hell'}}<crc8>\0
 *
 * crc8:
 *   hello/x{name:'yon',age:'18',equip:['hat','shoes']} => crc8 is 0x13
 *   hello/y{name:'myu',age:'12',equip:['gun','bomb']} => crc8 is 0xcc
 */

#define SRRP_REQUEST_LEADER '>'
#define SRRP_RESPONSE_LEADER '<'
#define SRRP_BEGIN_PACKET '^'
#define SRRP_MID_PACKET '0'
#define SRRP_END_PACKET '$'
#define SRRP_DELIMITER ':'
#define SRRP_SEQNO_HIGH 966
#define SRRP_LENGTH_MAX 4096

#define ESIZE 0
#define EFORMAT 1
#define ELEN 2

struct srrp_packet {
    char leader;
    int serial;
    char seat;
    size_t len;
    char *header;
    char *data;
};

int /*nr*/
srrp_read_one_packet(const char *buf, size_t size, struct srrp_packet /*out*/ *pac);
int /*nr*/
srrp_write_request(char *buf, size_t size, const char *header, const char *data);
int /*nr*/
srrp_write_response(char *buf, size_t size, const char *header, const char *data);

#ifdef __cplusplus
}
#endif
#endif
