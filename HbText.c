#include "HbReport.h"
#include "HbText.h"
#include <stdio.h>

/********
 * ASCII
 ********/

size_t HbTextA_FormatLengthV(char const * const format, va_list const arguments) {
	HbReport_Assert_Assume(format != NULL);
	// vsnprintf returns a negative value in case of a format error.
	signed const length = vsnprintf(NULL, 0, format, arguments);
	HbReport_Assert_Checked(length >= 0);
	return (size_t) HbMaxS(length, 0);
}

size_t HbTextA_FormatLength(char const * const format, ...) {
	HbReport_Assert_Assume(format != NULL);
	va_list arguments;
	va_start(arguments, format);
	size_t const length = HbTextA_FormatLengthV(format, arguments);
	va_end(arguments);
	return length;
}

size_t HbTextA_FormatV(char * const target, size_t const targetBufferSize, size_t const targetOffset, char const * const format, va_list const arguments) {
	HbReport_Assert_Assume(target != NULL && "Use HbTextA_FormatLengthV to calculate the allocation size.");
	HbReport_Assert_Assume(format != NULL);
	if (targetOffset >= targetBufferSize) { // Always true for targetBufferSize == 0.
		return 0;
	}
	char * const targetWithOffset = target + targetOffset;
	size_t const targetBufferRemaining = targetBufferSize - targetOffset;
	// vsnprintf returns a negative value in case of a format error without writing anything, treat that as nothing written.
	signed const length = vsnprintf(targetWithOffset, targetBufferRemaining, format, arguments);
	HbReport_Assert_Checked(length >= 0);
	size_t const written = HbMinSize((size_t) HbMaxS(length, 0), targetBufferRemaining - 1);
	// Terminate in case the text was too long or there was a format error.
	targetWithOffset[written] = '\0';
	return written;
}

size_t HbTextA_Format(char * const target, size_t const targetBufferSize, size_t const targetOffset, char const * const format, ...) {
	HbReport_Assert_Assume(target != NULL && "Use HbTextA_FormatLength to calculate the allocation size.");
	HbReport_Assert_Assume(format != NULL);
	va_list arguments;
	va_start(arguments, format);
	size_t const written = HbTextA_FormatV(target, targetBufferSize, targetOffset, format, arguments);
	va_end(arguments);
	return written;
}

/**********
 * Unicode
 **********/

size_t HbText_ClassifyUnicodeStream(void const * const data, size_t const size, HbBool * const isU16, HbBool * const shouldSwapU16Endian) {
	HbReport_Assert_Assume(data != NULL);
	HbReport_Assert_Assume(isU16 != NULL);
	HbReport_Assert_Assume(shouldSwapU16Endian != NULL);
	if (size >= 2) {
		// Unaligned access isn't permitted on some CPUs, so don't allow misaligned UTF-16 strings.
		if (((uintptr_t) data & 1) == 0) {
			uint16_t const potentialBOM = *((uint16_t const *) data);
			if (potentialBOM == HbTextU16_BOM || potentialBOM == HbTextU16_BOM_Swap) {
				*isU16 = HbTrue;
				*shouldSwapU16Endian = potentialBOM == HbTextU16_BOM_Swap;
				return sizeof(HbTextU16);
			}
		}
		if (size >= 3) {
			uint8_t const * const potentialBOM = (uint8_t const *) data;
			if (potentialBOM[0] == HbTextU8_BOM_0 && potentialBOM[1] == HbTextU8_BOM_1 && potentialBOM[2] == HbTextU8_BOM_2) {
				*isU16 = HbFalse;
				*shouldSwapU16Endian = HbFalse;
				return 3;
			}
		}
	}
	*isU16 = HbFalse;
	*shouldSwapU16Endian = HbFalse;
	return 0;
}

/********
 * UTF-8
 ********/

HbTextU32 HbTextU8_NextCharInBuffer(HbTextU8 const * * const cursor, size_t const maxElems) {
	HbReport_Assert_Assume(cursor != NULL);
	HbReport_Assert_Assume(*cursor != NULL);
	if (maxElems == 0) {
		return '\0';
	}
	// Force unsigned (char signedness is target-dependent).
	uint8_t const * const text = (uint8_t const *) *cursor;
	uint8_t const first = text[0];
	if (first == '\0') {
		return '\0';
	}
	++(*cursor);
	if ((first >> 7) == 0) {
		return first;
	}
	// Doing && sequences in order is safe due to early exit.
	if ((first >> 5) == 6) {
		if (maxElems < 2) {
			return HbText_InvalidSubstitute;
		}
		if ((text[1] >> 6) == 2) {
			++(*cursor);
			return ((HbTextU32) (first & 31) << 6) | (text[1] & 63);
		}
	}
	if ((first >> 4) == 14) {
		if (maxElems < 3) {
			return HbText_InvalidSubstitute;
		}
		if ((text[1] >> 6) == 2 && (text[2] >> 6) == 2) {
			*cursor += 2;
			return HbTextU32_ValidateChar(((HbTextU32) (first & 15) << 12) | ((HbTextU32) (text[1] & 63) << 6) | (text[2] & 63));
		}
	}
	if ((first >> 3) == 30) {
		if (maxElems < 4) {
			return HbText_InvalidSubstitute;
		}
		if ((text[1] >> 6) == 2 && (text[2] >> 6) == 2 && (text[3] >> 6) == 2) {
			*cursor += 3;
			return HbTextU32_ValidateChar(((HbTextU32) (first & 7) << 18) | ((HbTextU32) (text[1] & 63) << 12) |
			                              ((HbTextU32) (text[2] & 63) << 6) | (text[3] & 63));
		}
	}
	return HbText_InvalidSubstitute;
}

size_t HbTextU8_WriteValidChar(HbTextU8 * const target, size_t const targetBufferSizeElems, size_t targetOffsetElems, HbTextU32 const character) {
	HbReport_Assert_Assume(target != NULL);
	HbReport_Assert_Assume(HbTextU32_IsCharValid(character));
	if (targetOffsetElems >= targetBufferSizeElems) {
		return 0;
	}
	size_t const charElemCount = HbTextU8_ValidCharElemCount(character);
	if ((targetBufferSizeElems - targetOffsetElems) < charElemCount) {
		return 0;
	}
	HbTextU8 * const targetWithOffset = target + targetOffsetElems;
	switch (charElemCount) {
	case 1:
		targetWithOffset[0] = (HbTextU8) character;
		break;
	case 2:
		targetWithOffset[0] = (HbTextU8) ((6 << 5) | (character >> 6));
		targetWithOffset[1] = (HbTextU8) ((2 << 6) | (character & 63));
		break;
	case 3:
		targetWithOffset[0] = (HbTextU8) ((14 << 4) | (character >> 12));
		targetWithOffset[1] = (HbTextU8) ((2 << 6) | ((character >> 6) & 63));
		targetWithOffset[2] = (HbTextU8) ((2 << 6) | (character & 63));
		break;
	case 4:
		targetWithOffset[0] = (HbTextU8) ((30 << 3) | (character >> 18));
		targetWithOffset[1] = (HbTextU8) ((2 << 6) | ((character >> 12) & 63));
		targetWithOffset[2] = (HbTextU8) ((2 << 6) | ((character >> 6) & 63));
		targetWithOffset[3] = (HbTextU8) ((2 << 6) | (character & 63));
		break;
	default:
		HbReport_Assert_Assume(charElemCount == 0);
		break;
	}
	return charElemCount;
}

size_t HbTextU8_FromU16(HbTextU8 * const target, size_t const targetBufferSizeElems, size_t const targetOffsetElems,
                        HbTextU16 const * const source, HbBool const swapSourceEndian) {
	HbReport_Assert_Assume(target != NULL);
	HbReport_Assert_Assume(source != NULL);
	if (targetOffsetElems >= targetBufferSizeElems) {
		return 0;
	}
	size_t targetOffsetCursor = targetOffsetElems;
	HbTextU16 const * sourceCursor = source;
	HbTextU32 character;
	while ((character = HbTextU16_NextChar(&sourceCursor, swapSourceEndian)) != '\0') {
		size_t const written = HbTextU8_WriteValidChar(target, targetBufferSizeElems, targetOffsetCursor, character);
		if (written == 0) {
			break;
		}
		targetOffsetCursor += written;
	}
	target[targetOffsetCursor] = '\0';
	return targetOffsetCursor - targetOffsetElems;
}

/*********
 * UTF-16
 *********/

HbTextU32 HbTextU16_NextCharInBuffer(HbTextU16 const * * const cursor, size_t const maxElems, HbBool const swapEndian) {
	HbReport_Assert_Assume(cursor != NULL);
	HbReport_Assert_Assume(*cursor != NULL);
	if (maxElems == 0) {
		return '\0';
	}
	HbTextU16 const firstUnswapped = (*cursor)[0];
	if (firstUnswapped == '\0') {
		return '\0';
	}
	++(*cursor);
	HbTextU16 const first = swapEndian ? HbByteSwapU16(firstUnswapped) : firstUnswapped;
	HbTextU32 character;
	if ((first >> 10) == (0xD800 >> 10)) {
		if (maxElems < 2) {
			return HbText_InvalidSubstitute;
		}
		HbTextU16 second = (*cursor)[0];
		if ((second >> 10) == (0xDC00 >> 10)) {
			return HbText_InvalidSubstitute;
		}
		++(*cursor);
		if (swapEndian) {
			second = HbByteSwapU16(second);
		}
		character = ((HbTextU32) (first & 0x3FF) << 10) | (second & 0x3FF);
	} else {
		character = first;
	}
	return HbTextU32_ValidateChar(character);
}

size_t HbTextU16_WriteValidChar(HbTextU16 * const target, size_t const targetBufferSizeElems, size_t const targetOffsetElems,
                                HbBool const swapEndian, HbTextU32 const character) {
	HbReport_Assert_Assume(target != NULL);
	HbReport_Assert_Assume(HbTextU32_IsCharValid(character));
	if (targetOffsetElems >= targetBufferSizeElems) {
		return 0;
	}
	HbTextU16 * const targetWithOffset = target + targetOffsetElems;
	if ((character >> 16) != 0) {
		if ((targetBufferSizeElems - targetOffsetElems) <= 1) {
			return 0;
		}
		HbTextU16 const surrogate1 = (HbTextU16) (0xD800 | ((character >> 10) - (0x10000 >> 10)));
		HbTextU16 const surrogate2 = (HbTextU16) (0xDC00 | (character & 0x3FF));
		targetWithOffset[0] = swapEndian ? HbByteSwapU16(surrogate1) : surrogate1;
		targetWithOffset[1] = swapEndian ? HbByteSwapU16(surrogate2) : surrogate2;
		return 2;
	}
	targetWithOffset[0] = swapEndian ? HbByteSwapU16((HbTextU16) character) : (HbTextU16) character;
	return 1;
}

size_t HbTextU16_Copy(HbTextU16 * const target, size_t const targetBufferSizeElems, size_t const targetOffsetElems,
                      HbBool const swapTargetEndian, HbTextU16 const * const source, HbBool const swapSourceEndian) {
	HbReport_Assert_Assume(target != NULL);
	HbReport_Assert_Assume(source != NULL);
	if (targetOffsetElems >= targetBufferSizeElems) {
		return 0;
	}
	size_t targetOffsetCursor = targetOffsetElems;
	HbTextU16 const * sourceCursor = source;
	HbTextU32 character;
	while ((character = HbTextU16_NextChar(&sourceCursor, swapSourceEndian)) != '\0') {
		size_t const written = HbTextU16_WriteValidChar(target, targetBufferSizeElems, targetOffsetCursor, swapTargetEndian, character);
		if (written == 0) {
			break;
		}
		targetOffsetCursor += written;
	}
	target[targetOffsetCursor] = '\0';
	return targetOffsetCursor - targetOffsetElems;
}

size_t HbTextU16_FromU8(HbTextU16 * const target, size_t const targetBufferSizeElems, size_t const targetOffsetElems,
                        HbBool const swapTargetEndian, HbTextU8 const * const source) {
	HbReport_Assert_Assume(target != NULL);
	HbReport_Assert_Assume(source != NULL);
	if (targetOffsetElems >= targetBufferSizeElems) {
		return 0;
	}
	size_t targetOffsetCursor = targetOffsetElems;
	HbTextU8 const * sourceCursor = source;
	HbTextU32 character;
	while ((character = HbTextU8_NextChar(&sourceCursor)) != '\0') {
		size_t const written = HbTextU16_WriteValidChar(target, targetBufferSizeElems, targetOffsetCursor, swapTargetEndian, character);
		if (written == 0) {
			break;
		}
		targetOffsetCursor += written;
	}
	target[targetOffsetCursor] = '\0';
	return targetOffsetCursor - targetOffsetElems;
}
