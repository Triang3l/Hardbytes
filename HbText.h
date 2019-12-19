#ifndef HbInclude_HbText
#define HbInclude_HbText
#include "HbReport.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Zero-terminated string manipulation.
 *
 * "Element" (elem) is a single char, HbTextU8, HbTextU16, HbTextU32.
 * Counts of elements are used for offsets, buffer sizes.
 * "Character" (char) is a whole ASCII or Unicode code point, which may consists of multiple elements in UTF-8 and UTF-16.
 * It's used for iteration, for displaying.
 *
 * "Buffer size" in function prototypes includes the terminating element, "length" doesn't.
 *
 * Functions that may be useful for concatenation (copying, formatting) return the length of what they've written in elements usually,
 * not including the zero terminator, and may be chained by adding the result to the next offset.
 * They could also be returning the new offset, but it's slightly nicer to compare to zero to check for buffer end.
 */

typedef char HbTextU8; // So ASCII and UTF-8 literals can be written the same way.
typedef uint16_t HbTextU16;
typedef uint32_t HbTextU32; // Whole code point.

// Replacement character for anything invalid - must be a 7-bit ASCII character.
#define HbText_InvalidSubstitute '?'

// Maximum lengths of integers converted to strings.
#define HbText_Decimal_MaxLengthU8  3  // strlen("255")
#define HbText_Decimal_MaxLengthS8  4  // strlen("-128")
#define HbText_Decimal_MaxLengthU16 5  // strlen("65535")
#define HbText_Decimal_MaxLengthS16 6  // strlen("-32768")
#define HbText_Decimal_MaxLengthU32 10 // strlen("4294967295")
#define HbText_Decimal_MaxLengthS32 11 // strlen("-2147483648")
#define HbText_Decimal_MaxLengthU64 20 // strlen("18446744073709551615")
#define HbText_Decimal_MaxLengthS64 20 // strlen("-9223372036854775808")
#if UINT_MAX == UINT32_MAX
#define HbText_Decimal_MaxLengthUI HbText_Decimal_MaxLengthU32
#else
#error HbText_Decimal_MaxLengthUI: Couldn't pick from known integer lengths.
#endif
#if INT_MAX == INT32_MAX
#define HbText_Decimal_MaxLengthSI HbText_Decimal_MaxLengthS32
#else
#error HbText_Decimal_MaxLengthSI: Couldn't pick from known integer lengths.
#endif
#if SIZE_MAX == UINT64_MAX
#define HbText_Decimal_MaxLength_Size HbText_Decimal_MaxLength_U64
#elif SIZE_MAX == UINT32_MAX
#define HbText_Decimal_MaxLength_Size HbText_Decimal_MaxLength_U32
#else
#error HbText_Decimal_MaxLength_Size: Couldn't pick from known integer lengths.
#endif

/********
 * ASCII
 ********/

HbForceInline HbBool HbTextA_IsSpace(char const character) {
	return character == ' ' || (character >= '\t' && character <= '\r');
}
#define HbTextA_CharToLower_Generic(character) (((character) >= 'A' && (character) <= 'Z') ? ((character) + ('a' - 'A')) : (character))
#define HbTextA_CharToUpper_Generic(character) (((character) >= 'a' && (character) <= 'z') ? ((character) - ('a' - 'A')) : (character))
HbForceInline char HbTextA_CharToLower(char const character) { return HbTextA_CharToLower_Generic(character); }
HbForceInline char HbTextA_CharToUpper(char const character) { return HbTextA_CharToUpper_Generic(character); }

#define HbTextA_Compare_Func strcmp // One less indirection for when it's passed to qsort/bsearch.
#define HbTextA_CompareStarts_Func strncmp
#if defined(HbPlatform_OS_Microsoft)
#define HbTextA_CompareCaseless_Func _stricmp
#define HbTextA_CompareStartsCaseless_Func _strnicmp
#else
#define HbTextA_CompareCaseless_Func strcasecmp
#define HbTextA_CompareStartsCaseless_Func strncasecmp
#endif
HbForceInline size_t HbTextA_Length(char const * const text) {
	HbReport_Assert_Assume(text != NULL);
	return strlen(text);
}
HbForceInline signed HbTextA_Compare(char const * const text1, char const * const text2) {
	HbReport_Assert_Assume(text1 != NULL);
	HbReport_Assert_Assume(text2 != NULL);
	return HbTextA_Compare_Func(text1, text2);
}
HbForceInline signed HbTextA_CompareStarts(char const * const text1, char const * const text2, size_t const length) {
	HbReport_Assert_Assume(text1 != NULL);
	HbReport_Assert_Assume(text2 != NULL);
	return HbTextA_CompareStarts_Func(text1, text2, length);
}
HbForceInline signed HbTextA_CompareCaseless(char const * const text1, char const * const text2) {
	HbReport_Assert_Assume(text1 != NULL);
	HbReport_Assert_Assume(text2 != NULL);
	return HbTextA_CompareCaseless_Func(text1, text2);
}
HbForceInline signed HbTextA_CompareStartsCaseless(char const * const text1, char const * const text2, size_t const length) {
	HbReport_Assert_Assume(text1 != NULL);
	HbReport_Assert_Assume(text2 != NULL);
	return HbTextA_CompareStartsCaseless_Func(text1, text2, length);
}

inline size_t HbTextA_Copy(char * const target, size_t const targetBufferSize, size_t const targetOffset, char const * const source) {
	HbReport_Assert_Assume(target != NULL);
	HbReport_Assert_Assume(source != NULL);
	if (targetOffset >= targetBufferSize) { // Always true for targetBufferSize == 0.
		return 0;
	}
	char * const targetWithOffset = target + targetOffset;
	char * targetCursor = target;
	size_t targetRemaining = targetBufferSize - 1 - targetOffset;
	char const * sourceCursor = source;
	while (targetRemaining != 0 && *sourceCursor != '\0') {
		*(targetCursor++) = *(sourceCursor++);
		--targetRemaining;
	}
	*targetCursor = '\0';
	return (size_t) (targetCursor - target);
}

size_t HbTextA_FormatLengthV(char const * const format, va_list const arguments);
size_t HbTextA_FormatLength(char const * const format, ...);
size_t HbTextA_FormatV(char * const target, size_t const targetBufferSize, size_t const targetOffset, char const * const format, va_list const arguments);
size_t HbTextA_Format(char * const target, size_t const targetBufferSize, size_t const targetOffset, char const * const format, ...);

/*******************************************************************
 * Common Unicode
 * Not very strict handling, just storage and conversion safeguards
 *******************************************************************/

inline HbBool HbTextU32_IsCharValid(HbTextU32 const character) {
	// Allow only characters that can be stored in UTF-8 and UTF-16, disallow BOM and surrogates.
	return character <= 0x10FFFF && character != 0xFEFF && (character & 0xFFFE) != 0xFFFE && (character & ~((HbTextU32) 0x7FF)) != 0xD800;
}
HbForceInline HbTextU32 HbTextU32_ValidateChar(HbTextU32 const character) {
	return HbTextU32_IsCharValid(character) ? character : HbText_InvalidSubstitute;
}

HbForceInline HbBool HbTextU32_IsASCIISpace(HbTextU32 const character) {
	return character == ' ' || (character >= '\t' && character <= '\r');
}
HbForceInline HbTextU32 HbTextU32_ASCIICharToLower(HbTextU32 const character) { return HbTextA_CharToLower_Generic(character); }
HbForceInline HbTextU32 HbTextU32_ASCIICharToUpper(HbTextU32 const character) { return HbTextA_CharToUpper_Generic(character); }

// Returns the number of BOM bytes to skip. For UTF-16 to be detectable, data must be 2-aligned.
size_t HbText_ClassifyUnicodeStream(void const * const data, size_t const size, HbBool * const isU16, HbBool * const shouldSwapU16Endian);

/*************************************************************************
 * UTF-8 - assuming no incomplete characters
 * (invalid characters are treated as sequences of substitute characters)
 *************************************************************************/

#define HbTextU8_BOM_0 0xEF
#define HbTextU8_BOM_1 0xBB
#define HbTextU8_BOM_2 0xBF

#define HbTextU8_MaxCharElems 4

inline size_t HbTextU8_ValidCharElemCount(HbTextU32 const character) {
	HbReport_Assert_Assume(HbTextU32_IsCharValid(character));
	return (character > 0) + (character > 0x7F) + (character > 0x7FF) + (character > 0xFFFF);
}
inline size_t HbTextU8_CharElemCount(HbTextU32 const character) {
	return HbTextU32_IsCharValid(character) ? HbTextU8_ValidCharElemCount(character) : 1;
}

// 0 when no characters left. Advances the cursor.
// maxElems is for non-null-terminated buffers, to prevent buffer overflow when the last character is truncated.
// It should be HbTextU8_MaxCharElems for null-terminated strings.
HbTextU32 HbTextU8_NextCharInBuffer(HbTextU8 const * * const cursor, size_t const maxElems);
HbForceInline HbTextU32 HbTextU8_NextChar(HbTextU8 const * * const cursor) {
	return HbTextU8_NextCharInBuffer(cursor, HbTextU8_MaxCharElems);
}
#define HbTextU8_LengthElems HbTextA_Length
inline size_t HbTextU8_LengthChars(HbTextU8 const * const text) {
	HbReport_Assert_Assume(text != NULL);
	size_t length = 0;
	HbTextU8 const * cursor = text;
	while (HbTextU8_NextChar(&cursor) != '\0') {
		++length;
	}
	return length;
};
inline size_t HbTextU8_LengthU16Elems(HbTextU8 const * const text) {
	HbReport_Assert_Assume(text != NULL);
	size_t length = 0;
	HbTextU8 const * cursor = text;
	HbTextU32 character;
	while ((character = HbTextU8_NextChar(&cursor)) != '\0') {
		length += 1 + ((character >> 16) != 0);
	}
	return length;
}

#define HbTextU8_Compare_Func HbTextA_Compare_Func
#define HbTextU8_Compare HbTextA_Compare
#define HbTextU8_CompareCaselessEnglish_Func HbTextA_CompareCaseless_Func
#define HbTextU8_CompareCaselessEnglish HbTextA_CompareCaseless

// Places a character in the buffer if possible, returning the number of elements actually written.
// Does not null-terminate (and the buffer size doesn't include the terminator)!
size_t HbTextU8_WriteValidChar(HbTextU8 * const target, size_t const targetBufferSizeElems, size_t targetOffsetElems, HbTextU32 const character);

// Allocate a null-terminated string with HbTextU16_LengthU8Elems elements for this.
size_t HbTextU8_FromU16(HbTextU8 * const target, size_t const targetBufferSizeElems, size_t const targetOffset,
                        HbTextU16 const * const source, HbBool const swapSourceEndian);

/*********
 * UTF-16
 *********/

#define HbTextU16_BOM 0xFEFF
#define HbTextU16_BOM_Swap 0xFFFE

#define HbTextU16_MaxCharElems 2

// 0 when no characters left. Advances the cursor.
// maxElems is for non-null-terminated buffers, to prevent buffer overflow when the last character is truncated.
// It should be HbTextU16_MaxCharElems for null-terminated strings.
HbTextU32 HbTextU16_NextCharInBuffer(HbTextU16 const * * const cursor, size_t const maxElems, HbBool const swapEndian);
HbForceInline HbTextU32 HbTextU16_NextChar(HbTextU16 const * * const cursor, HbBool const swapEndian) {
	return HbTextU16_NextCharInBuffer(cursor, HbTextU16_MaxCharElems, swapEndian);
}

// No validation - returns real data size (for appending)!
inline size_t HbTextU16_LengthElems(HbTextU16 const * const text) {
	HbReport_Assert_Assume(text != NULL);
	HbTextU16 const * cursor = text;
	while (*(cursor++) != '\0') {}
	return (size_t) (cursor - text);
}
inline size_t HbTextU16_LengthChars(HbTextU16 const * const text, HbBool const swapEndian) {
	HbReport_Assert_Assume(text != NULL);
	size_t length = 0;
	HbTextU16 const * cursor = text;
	while (HbTextU16_NextChar(&cursor, swapEndian) != '\0') {
		++length;
	}
	return length;
};
inline size_t HbTextU16_LengthU8Elems(HbTextU16 const * const text, HbBool const swapEndian) {
	HbReport_Assert_Assume(text != NULL);
	size_t length = 0;
	HbTextU16 const * cursor = text;
	HbTextU32 character;
	while ((character = HbTextU16_NextChar(&cursor, swapEndian)) != '\0') {
		length += HbTextU8_ValidCharElemCount(character);
	}
	return length;
};

// Places a character in the buffer if possible, returning the number of elements actually written.
// Does not null-terminate (and the buffer size doesn't include the terminator)!
size_t HbTextU16_WriteValidChar(HbTextU16 * const target, size_t const targetBufferSizeElems, size_t const targetOffsetElems,
                                HbBool const swapEndian, HbTextU32 const character);

// This actually validates characters - size in elements may be changed if there are invalid characters!
size_t HbTextU16_Copy(HbTextU16 * const target, size_t const targetBufferSizeElems, size_t const targetOffsetElems,
                      HbBool const swapTargetEndian, HbTextU16 const * const source, HbBool const swapSourceEndian);

// Allocate a null-terminated string with HbTextU8_LengthU16Elems elements for this.
size_t HbTextU16_FromU8(HbTextU16 * const target, size_t const targetBufferSizeElems, size_t const targetOffsetElems,
                        HbBool const swapTargetEndian, HbTextU8 const * const source);

#ifdef __cplusplus
}
#endif
#endif
