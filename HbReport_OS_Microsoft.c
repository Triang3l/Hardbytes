#include "HbCommon.h"
#ifdef HbPlatform_OS_Microsoft
#include "HbMath.h"
#include "HbReport.h"
#include "HbText.h"
#include <stdio.h>
#include <Windows.h>

// Allocations are not safe in crash functions and may be huge for both crashes and messages, use the MsgBox limit from VBScript.

#ifdef HbReport_Build_Assert
HbNoReturn void HbReport_OS_AssertCrash(char const * const function, unsigned const line, char const * const statement, HbBool isAssumption) {
	char message[1024];
	message[HbMath_MinSize((size_t) HbMath_MaxS(snprintf(message, HbCountOf(message),
	                                                     "%s:%u (%s): %s", function, line, isAssumption ? "strictly assumed" : "undesirable", statement),
	                                            0), HbCountOf(message) - 1)] = '\0';
	OutputDebugStringA("Hardbytes assertion failed: ");
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
	#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	MessageBoxA(NULL, message, "Hardbytes assertion failed", MB_OK | MB_ICONERROR);
	#else
	#error HbReport_OS_AssertCrash: No implementation for the target Windows API family partition.
	#endif
	HbReport_Break();
	_exit(EXIT_FAILURE);
}
#endif

HbNoReturn void HbReport_OS_CrashV(char const * const function, unsigned const line, char const * const format, va_list const arguments) {
	size_t const prefixLength = (size_t) HbMath_MaxS(snprintf(NULL, 0, "%s:%u: ", function, line), 0);
	size_t const messageLength = HbMath_MinSize((size_t) HbMath_MaxS(vsnprintf(NULL, 0, format, arguments), 0), 1023);
	char * const message = HbStackAlloc(char, prefixLength + messageLength + 1);
	snprintf(message, prefixLength + 1, "%s:%u: ", function, line);
	vsnprintf(message + prefixLength, messageLength + 1, format, arguments);
	message[prefixLength + messageLength] = '\0';
	OutputDebugStringA("Hardbytes fatal error: ");
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
	#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	MessageBoxA(NULL, message, "Hardbytes fatal error", MB_OK | MB_ICONERROR);
	#else
	#error HbReport_OS_CrashV: No implementation for the target Windows API family partition.
	#endif
	HbReport_Break();
	_exit(EXIT_FAILURE);
}

#ifdef HbReport_Build_Message
void HbReport_OS_MessageV(char const * const format, va_list const arguments) {
	HbReport_Assert_Assume(format != NULL);
	char message[1024];
	HbTextA_FormatV(message, HbCountOf(message), 0, format, arguments);
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
}
#endif

#endif
