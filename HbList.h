#ifndef HbInclude_HbList
#define HbInclude_HbList
#include "HbReport.h"
#ifdef __cplusplus
extern "C" {
#endif

// Prepend(element, first, last, prev, next) == Append(element, last, first, next, prev), but separate for cleaner assertions.

#define HbList_2Way_Prepend(elementPointer, firstPointer, lastPointer, prevField, nextField)\
do {\
	(elementPointer)->prevField = NULL;\
	(elementPointer)->nextField = (firstPointer);\
	if ((firstPointer) != NULL) {\
		HbReport_Assert_Assume((lastPointer) != NULL);\
		HbReport_Assert_Assume((firstPointer)->prevField == NULL);\
		(firstPointer)->prevField = (elementPointer);\
	} else {\
		HbReport_Assert_Assume((lastPointer) == NULL);\
		(lastPointer) = (elementPointer);\
	}\
	(firstPointer) = (elementPointer);\
} while (HbFalse)

#define HbList_2Way_Append(elementPointer, firstPointer, lastPointer, prevField, nextField)\
do {\
	(elementPointer)->prevField = (lastPointer);\
	(elementPointer)->nextField = NULL;\
	if ((lastPointer) != NULL) {\
		HbReport_Assert_Assume((firstPointer) != NULL);\
		HbReport_Assert_Assume((lastPointer)->nextField == NULL);\
		(lastPointer)->nextField = (elementPointer);\
	} else {\
		HbReport_Assert_Assume((firstPointer) == NULL);\
		(firstPointer) = (elementPointer);\
	}\
	(lastPointer) = (elementPointer);\
} while (HbFalse)

#define HbList_2Way_Unlink(elementPointer, firstPointer, lastPointer, prevField, nextField)\
do {\
	if ((elementPointer)->prevField != NULL) {\
		HbReport_Assert_Assume((firstPointer) != (elementPointer));\
		(elementPointer)->prevField->nextField = (elementPointer)->nextField;\
	} else {\
		HbReport_Assert_Assume((firstPointer) == (elementPointer));\
		(firstPointer) = (elementPointer)->nextField;\
	}\
	if ((elementPointer)->nextField != NULL) {\
		HbReport_Assert_Assume((lastPointer) != (elementPointer));\
		(elementPointer)->nextField->prevField = (elementPointer)->prevField;\
	} else {\
		HbReport_Assert_Assume((lastPointer) == (elementPointer));\
		(lastPointer) = (elementPointer)->prevField;\
	}\
} while (HbFalse)

#ifdef __cplusplus
}
#endif
#endif
