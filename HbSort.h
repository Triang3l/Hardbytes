#ifndef HbInclude_HbSort
#define HbInclude_HbSort
#include "HbReport.h"
#ifdef __cplusplus
extern "C" {
#endif

// Search functions return elementCount if not found.

#define HbSort_Find_Equal_CreateForNumberArray(name, type)\
inline type name(type const value, type const * const elements, size_t const elementCount) {\
	HbReport_Assert_Assume(elements != NULL);\
	size_t start = 0, end = elementCount;\
	while (start < end) {\
		size_t const middle = start + (end - 1 - start) / 2;\
		type middleValue = elements[middle];\
		if (middleValue > value) {\
			end = middle;\
		} else if (middleValue < value) {\
			start = middle + 1;\
		} else {\
			return middle;\
		}\
	}\
	return elementCount;\
}
HbSort_Find_Equal_CreateForNumberArray(HbSort_Find_Equal_Size, size_t)

#define HbSort_Find_FirstNotLess_CreateForNumberArray(name, type)\
inline type name(type const value, type const * const elements, size_t const elementCount) {\
	HbReport_Assert_Assume(elements != NULL);\
	size_t start = 0, end = elementCount;\
	while (start < end) {\
		size_t const middle = start + (end - 1 - start) / 2;\
		if (elements[middle] >= value) {\
			end = middle;\
		} else {\
			start = middle + 1;\
		}\
	}\
	return start;\
}
HbSort_Find_FirstNotLess_CreateForNumberArray(HbSort_Find_FirstNotLess_Size, size_t)

#define HbSort_Find_FirstGreater_CreateForNumberArray(name, type)\
inline type name(type const value, type const * const elements, size_t const elementCount) {\
	HbReport_Assert_Assume(elements != NULL);\
	size_t start = 0, end = elementCount;\
	while (start < end) {\
		size_t const middle = start + (end - 1 - start) / 2;\
		if (elements[middle] > value) {\
			end = middle;\
		} else {\
			start = middle + 1;\
		}\
	}\
	return start;\
}
HbSort_Find_FirstGreater_CreateForNumberArray(HbSort_Find_FirstGreater_Size, size_t)

#ifdef __cplusplus
}
#endif
#endif
