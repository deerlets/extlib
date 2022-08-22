#include "srrp.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "stddefx.h"
#include "crc16.h"

#define SEQNO_MAX_LEN 32
#define LENGTH_MAX_LEN 32
#define REQID_MAX_LEN 32

struct srrp_packet *__srrp_read_one_request(const char *buf, size_t size)
{
    assert(buf[0] == SRRP_REQUEST_LEADER);

    char leader, seat;
    uint32_t seqno, len, reqid;

    // FIXME: shall we use "%c%x,%c,%4x,%4x:%[^{}]%s" to parse header and data ?
    int cnt = sscanf(buf, "%c%x,%c,%4x,%4x:", &leader, &seqno, &seat, &len, &reqid);
    if (cnt != 5) return NULL;

    const char *header_delimiter = strstr(buf, ":/");
    const char *data_delimiter = strstr(buf, "?{");
    if (header_delimiter == NULL || data_delimiter == NULL)
        return NULL;

    struct srrp_packet *pac = calloc(1, sizeof(*pac) + len);
    memcpy(pac->raw, buf, len);
    pac->leader = leader;
    pac->seat = seat;
    pac->seqno = seqno;
    pac->len = len;
    pac->reqid = reqid;

    const char *header = header_delimiter + 1;
    const char *data = data_delimiter + 1;
    pac->header_len = data_delimiter - header;
    memcpy((void *)pac->header, header, pac->header_len);
    pac->data = pac->raw + (data - buf);
    pac->data_len = buf + strlen(buf) - data;

    int retval =  3 + 2 + 5 + 5 + pac->header_len + 1 + pac->data_len + 1/*stop*/;
    if (retval != pac->len) {
        free(pac);
        return NULL;
    }
    return pac;
}

struct srrp_packet *__srrp_read_one_response(const char *buf, size_t size)
{
    assert(buf[0] == SRRP_RESPONSE_LEADER);

    char leader, seat;
    uint32_t seqno, len, reqid, reqcrc16;

    // FIXME: shall we use "%c%x,%c,%4x,%4x:%x%[^{}]%s" to parse header and data ?
    int cnt = sscanf(buf, "%c%x,%c,%4x,%4x,%x:/",
                     &leader, &seqno, &seat, &len, &reqid, &reqcrc16);
    if (cnt != 6) return NULL;

    const char *header_delimiter = strstr(buf, ":/");
    const char *data_delimiter = strstr(buf, "?{");
    if (header_delimiter == NULL || data_delimiter == NULL)
        return NULL;

    struct srrp_packet *pac = calloc(1, sizeof(*pac) + len);
    memcpy(pac->raw, buf, len);
    pac->leader = leader;
    pac->seat = seat;
    pac->seqno = seqno;
    pac->len = len;
    pac->reqid = reqid;
    pac->reqcrc16 = reqcrc16;

    const char *header = header_delimiter + 1;
    const char *data = data_delimiter + 1;
    pac->header_len = data_delimiter - header;
    memcpy((void *)pac->header, header, pac->header_len);
    pac->data = pac->raw + (data - buf);
    pac->data_len = buf + strlen(buf) - data;

    int retval =  3 + 2 + 5 + 5 + 5 + pac->header_len + 1 + pac->data_len + 1/*stop*/;
    if (retval != pac->len) {
        free(pac);
        return NULL;
    }
    return pac;
}

struct srrp_packet *__srrp_read_one_subpub(const char *buf, size_t size)
{
    assert(buf[0] == SRRP_SUBSCRIBE_LEADER ||
           buf[0] == SRRP_UNSUBSCRIBE_LEADER ||
           buf[0] == SRRP_PUBLISH_LEADER);

    char leader, seat;
    uint32_t seqno, len;

    // FIXME: shall we use "%c%x,%c,%4x:%[^{}]%s" to parse header and data ?
    int cnt = sscanf(buf, "%c%x,%c,%4x:", &leader, &seqno, &seat, &len);
    if (cnt != 4) return NULL;

    const char *header_delimiter = strstr(buf, ":/");
    const char *data_delimiter = strstr(buf, "?{");
    if (header_delimiter == NULL || data_delimiter == NULL)
        return NULL;

    struct srrp_packet *pac = calloc(1, sizeof(*pac) + len);
    memcpy(pac->raw, buf, len);
    pac->leader = leader;
    pac->seat = seat;
    pac->seqno = seqno;
    pac->len = len;

    const char *header = header_delimiter + 1;
    const char *data = data_delimiter + 1;
    pac->header_len = data_delimiter - header;
    memcpy((void *)pac->header, header, pac->header_len);
    pac->data = pac->raw + (data - buf);
    pac->data_len = buf + strlen(buf) - data;

    int retval =  3 + 2 + 5 + pac->header_len + 1 + pac->data_len + 1/*stop*/;
    if (retval != pac->len) {
        return NULL;
    }
    return pac;
}

struct srrp_packet *srrp_read_one_packet(const char *buf, size_t size)
{
    const char *leader = buf;

    if (*leader == SRRP_REQUEST_LEADER)
        return __srrp_read_one_request(buf, size);
    else if (*leader == SRRP_RESPONSE_LEADER)
        return __srrp_read_one_response(buf, size);
    else if (*leader == SRRP_SUBSCRIBE_LEADER ||
             *leader == SRRP_UNSUBSCRIBE_LEADER ||
             *leader == SRRP_PUBLISH_LEADER)
        return __srrp_read_one_subpub(buf, size);

    return NULL;
}

void srrp_free(struct srrp_packet *pac)
{
    free(pac);
}

int srrp_write_request(
    char *buf, size_t size, uint16_t reqid,
    const char *header, const char *data)
{
    size_t len = 15 + strlen(header) + 1 + strlen(data) + 1/*stop*/;
    assert(len < SRRP_LENGTH_MAX - 4/*crc16*/);
    int nr = snprintf(buf, size, ">0,$,%.4x,%.4x:%s?%s",
                      (uint32_t)len, reqid, header, data);
    assert(nr + 1 == len);
    return len;
}

int srrp_write_response(
    char *buf, size_t size, uint16_t reqid, uint16_t reqcrc16,
    const char *header, const char *data)
{
    size_t len = 15 + 5/*crc16*/ + strlen(header) + 1 + strlen(data) + 1/*stop*/;
    assert(len < SRRP_LENGTH_MAX - 4/*crc16*/);
    int nr = snprintf(buf, size, "<0,$,%.4x,%.4x,%.4x:%s?%s",
                      (uint32_t)len, reqid, reqcrc16, header, data);
    assert(nr + 1 == len);
    return len;
}

int srrp_write_subscribe(char *buf, size_t size, const char *header, const char *ctrl)
{
    size_t len = 10 + strlen(header) + 1 + strlen(ctrl) + 1/*stop*/;
    assert(len < SRRP_LENGTH_MAX - 4/*crc16*/);
    int nr = snprintf(buf, size, "#0,$,%.4x:%s?%s", (uint32_t)len, header, ctrl);
    assert(nr + 1 == len);
    return len;
}

int /*nr*/ srrp_write_unsubscribe(
    char *buf, size_t size, const char *header)
{
    size_t len = 10 + strlen(header) + 1 + 2/*data*/ + 1/*stop*/;
    assert(len < SRRP_LENGTH_MAX - 4/*crc16*/);
    int nr = snprintf(buf, size, "%%0,$,%.4x:%s?{}", (uint32_t)len, header);
    assert(nr + 1 == len);
    return len;
}

int srrp_write_publish(char *buf, size_t size, const char *header, const char *data)
{
    size_t len = 10 + strlen(header) + 1 + strlen(data) + 1/*stop*/;
    assert(len < SRRP_LENGTH_MAX - 4/*crc16*/);
    int nr = snprintf(buf, size, "@0,$,%.4x:%s?%s", (uint32_t)len, header, data);
    assert(nr + 1 == len);
    return len;
}

int srrp_next_packet_offset(const char *buf)
{
    if (isdigit(buf[1])) {
        if (buf[0] == SRRP_REQUEST_LEADER ||
            buf[0] == SRRP_RESPONSE_LEADER ||
            buf[0] == SRRP_SUBSCRIBE_LEADER ||
            buf[0] == SRRP_UNSUBSCRIBE_LEADER ||
            buf[0] == SRRP_PUBLISH_LEADER) {
            return 0;
        }
    }

    char *p = NULL;

    p = strchr(buf, SRRP_REQUEST_LEADER);
    if (p && p[-1] != SRRP_REQUEST_LEADER && isdigit(p[1]))
        return p - buf;

    p = strchr(buf, SRRP_RESPONSE_LEADER);
    if (p && p[-1] != SRRP_RESPONSE_LEADER && isdigit(p[1]))
        return p - buf;

    p = strchr(buf, SRRP_SUBSCRIBE_LEADER);
    if (p && p[-1] != SRRP_SUBSCRIBE_LEADER && isdigit(p[1]))
        return p - buf;

    p = strchr(buf, SRRP_UNSUBSCRIBE_LEADER);
    if (p && p[-1] != SRRP_UNSUBSCRIBE_LEADER && isdigit(p[1]))
        return p - buf;

    p = strchr(buf, SRRP_PUBLISH_LEADER);
    if (p && p[-1] != SRRP_PUBLISH_LEADER && isdigit(p[1]))
        return p - buf;

    return -1;
}
