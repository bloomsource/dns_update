#ifndef _CRC32_H_
#define _CRC32_H_
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

uint32_t crc32( uint32_t crc, const void *buf, size_t size );

#ifdef __cplusplus
}
#endif

#endif

