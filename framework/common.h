#pragma once

#define USE_LIBUNWIND


int32_t decode_sleb128(uint8_t *sleb128);
uint32_t decode_uleb128(uint8_t *uleb128);
bool is_location_form(int form);
