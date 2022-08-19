#include "srrpx.h"
#include "stddefx.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define SERIAL_MAX_LEN 32
#define LENGTH_MAX_LEN 32

int srrp_read_one_packet(const char *buf, size_t size, struct srrp_packet *pac)
{
    const char *leader = buf;
    const char *serial = buf + 1;
    const char *seat = strchr(buf, ',');
    if (seat) seat += 1;
    const char *length = seat ? strchr(seat, ',') : NULL;
    if (length) length += 1;
    const char *delimiter = strchr(buf, SRRP_DELIMITER);
    const char *data_delimiter = strchr(buf, '{');

    if (strlen(buf) >= size) {
        errno = ESIZE;
        return -1;
    }

    if (seat == NULL || length == NULL || delimiter == NULL || data_delimiter == NULL) {
        errno = EFORMAT;
        return -1;
    }

    if (*leader != SRRP_REQUEST_LEADER && *leader != SRRP_RESPONSE_LEADER) {
        errno = EFORMAT;
        return -1;
    }

    if (*seat != SRRP_BEGIN_PACKET && *seat != SRRP_MID_PACKET && *seat != SRRP_END_PACKET) {
        errno = EFORMAT;
        return -1;
    }

    char tmp_length[LENGTH_MAX_LEN] = {0};
    memcpy(tmp_length, length, delimiter - length);
    char tmp_serial[SERIAL_MAX_LEN] = {0};
    memcpy(tmp_serial, serial, seat - serial - 1);

    if (atoi(tmp_length) != strlen(buf)) {
        errno = ELEN;
        return -1;
    }

    pac->leader = *leader;
    pac->serial = atoi(tmp_serial);
    pac->seat = *seat;
    pac->len = atoi(tmp_length);

    int len_header = data_delimiter - delimiter - 1;
    pac->header = malloc(len_header + 1);
    memset(pac->header, 0, len_header + 1);
    memcpy(pac->header, delimiter + 1, len_header);

    int len_data = buf + strlen(buf) - data_delimiter;
    pac->data = malloc(len_data + 1);
    memset(pac->data, 0, len_data + 1);
    memcpy(pac->data, data_delimiter, len_data);

    return strlen(buf) + 1;
}
