#ifndef _SOLARPATH_H_
#define _SOLARPATH_H_

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

const uint8_t *encode_packet(uint8_t *len);
void decode_packet(const uint8_t *packet, uint32_t len);
void system_update();

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus

#endif  // #ifndef _SOLARPATH_H_
