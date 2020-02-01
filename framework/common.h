#ifndef __PST_COMMON_H__
#define __PST_COMMON_H__

#include <stdint.h>

#define USE_LIBUNWIND

// TBD custom assertion if needed
#define pst_assert(expr) assert(expr)

// platform-dependent address size
#define PST_GENERIC_SIZE (8) // for x86_64 architecture

int32_t decode_sleb128(uint8_t *sleb128);
uint32_t decode_uleb128(uint8_t *uleb128);
bool is_location_form(int form);

#endif // __PST_COMMON_H__
