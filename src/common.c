/*
 * common.cpp
 *
 *  Created on: Jan 8, 2020
 *      Author: nnosov
 */

#include <inttypes.h>
#include <stdbool.h>

int32_t decode_sleb128(uint8_t *sleb128)
{
	int32_t num = 0, shift = 0, size = 0;
	do {
		num |= ((*sleb128 & 0x7f) << shift);
		shift += 7;
		size += 8;
	} while(*sleb128++ & 0x80);
	if((shift < size) && (*(--sleb128) & 0x40)) {
		num |= - (1 << shift);
	}
	return num;
}

uint32_t decode_uleb128(uint8_t *uleb128)
{
	uint32_t num = 0, shift = 0;
	do {
		num |= ((*uleb128 & 0x7f) << shift);
		shift += 7;
	} while(*uleb128++ & 0x80);
	return num;
}

// Utility function to encode a ULEB128 value to a buffer. Returns
// the length in bytes of the encoded value. by default PadTo = 0
inline unsigned encode_uleb128(uint64_t value, uint8_t *p, unsigned PadTo)
{
	uint8_t *orig_p = p;
	unsigned count = 0;
	do {
		uint8_t Byte = value & 0x7f;
		value >>= 7;
		count++;
		if (value != 0 || count < PadTo)
			Byte |= 0x80; // Mark this byte to show that more bytes will follow.
		*p++ = Byte;
	} while (value != 0);

	// Pad with 0x80 and emit a null byte at the end.
	if (count < PadTo) {
		for (; count < PadTo - 1; ++count)
			*p++ = '\x80';
		*p++ = '\x00';
	}

	return (unsigned)(p - orig_p);
}

// Utility function to encode a SLEB128 value to a buffer. Returns
// the length in bytes of the encoded value. by default PadTo = 0
inline unsigned encode_sleb128(int64_t value, uint8_t *p, unsigned PadTo)
{
	uint8_t *orig_p = p;
	unsigned count = 0;
	bool More;
	do {
		uint8_t Byte = value & 0x7f;
		// NOTE: this assumes that this signed shift is an arithmetic right shift.
		value >>= 7;
		More = !((((value == 0 ) && ((Byte & 0x40) == 0)) ||
				((value == -1) && ((Byte & 0x40) != 0))));
		count++;
		if (More || count < PadTo)
			Byte |= 0x80; // Mark this byte to show that more bytes will follow.
		*p++ = Byte;
	} while (More);

	// Pad with 0x80 and emit a terminating byte at the end.
	if (count < PadTo) {
		uint8_t PadValue = value < 0 ? 0x7f : 0x00;
		for (; count < PadTo - 1; ++count)
			*p++ = (PadValue | 0x80);
		*p++ = PadValue;
	}
	return (unsigned)(p - orig_p);
}

// Utility function to get the size of the ULEB128-encoded value.
unsigned getULEB128Size(uint64_t Value)
{
	unsigned Size = 0;
	do {
		Value >>= 7;
		Size += sizeof(int8_t);
	} while (Value);

	return Size;
}

// Utility function to get the size of the SLEB128-encoded value.
unsigned getSLEB128Size(int64_t Value)
{
	unsigned Size = 0;
	int Sign = Value >> (8 * sizeof(Value) - 1);
	bool IsMore;

	do {
		unsigned Byte = Value & 0x7f;
		Value >>= 7;
		IsMore = Value != Sign || ((Byte ^ Sign) & 0x40) != 0;
		Size += sizeof(int8_t);
	} while (IsMore);

	return Size;
}
