#ifndef __ETLV_DEBUG_H__
#define __ETLV_DEBUG_H__

#include <stdio.h>

extern void print_hex(const void* src, int len);

#define ETLV_PRINTF printf
#define ETLV_LOG_HEX print_hex

#endif /* __ETLV_DEBUG_H__ */
