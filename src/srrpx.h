#ifndef __SRRPX_H // simple request response protocol
#define __SRRPX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Request: >[seqno],[^|0|$],[lenth]:[header]{data}\0<crc16>\0
 *   >0,$,0061:/hello/x{name:'yon',age:'18',equip:['hat','shoes']}\0<crc16>\0
 *   >1,^,0013:/he\0<crc16>\0
 *   >2,0,0015:llo/y\0<crc16>\0
 *   >3,$,0052:{name:'myu',age:'12',equip:['gun','bomb']}\0<crc16>\0
 *
 * Response: <[seqno],[^|0|$],[lenth]:[crc16_req][header]{data}\0<crc16>\0
 *   <0,$,0062:<crc16>/hello/x{err:0,errmsg:'succ',data:{msg:'world'}}\0<crc16>\0
 *   <1,$,0061:<crc16>/hello/y{err:1,errmsg:'fail',data:{msg:'hell'}}\0<crc16>\0
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
    uint16_t crc16; // crc16_req when leader is '<'
    char *header;
    char *data;
};

int /*nr*/ srrp_read_one_packet(
    const char *buf, size_t size, struct srrp_packet /*out*/ *pac);
int /*nr*/ srrp_write_request(
    char *buf, size_t size, const char *header, const char *data);
int /*nr*/ srrp_write_response(
    char *buf, size_t size, uint16_t crc16_req,
    const char *header, const char *data);

#ifdef __cplusplus
}
#endif
#endif
