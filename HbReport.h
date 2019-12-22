#ifndef HbInclude_HbReport
#define HbInclude_HbReport
#include "HbCommon.h"
#if defined(HbPlatform_OS_Microsoft)
#include <intrin.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

// Rules of exception handling:
// - Significant internal logic error in the code (like null pointer) - assertion with a assumption.
// - Recoverable internal logic error - "checked" assertion and actual handling of the situation.
// - Failure of an external library, including failing to allocate pretty small objects - crash.
// - Corruption (including logic errors) in a file - recovery, or crash if it's a critical file.

// HbReport_Build_Debug/Assert/Profile may be enabled in the build configuration for more flexible settings.
#if !defined(HbReport_Build_Debug) && defined(_DEBUG)
#define HbReport_Build_Debug
#endif
#ifdef HbReport_Build_Debug
#ifndef HbReport_Build_Assert
#define HbReport_Build_Assert
#endif
#ifndef HbReport_Build_Message
#define HbReport_Build_Message
#endif
#ifndef HbReport_Build_Profile
#define HbReport_Build_Profile
#endif
#endif

#ifdef HbReport_Build_Assert
#if defined(HbPlatform_OS_Microsoft)
#define HbReport_Break __debugbreak
#else
#error HbReport_Break: No implementation for the target OS.
#endif
#else
#define HbReport_Break() HbNoOp
#endif

// Don't use HbText functions inside crashing functions because they may cause assertion failures and thus recursion.

// OS-specific.

#ifdef HbReport_Build_Assert
HbNoReturn void HbReport_OS_AssertCrash(char const * const function, unsigned const line, char const * const statement, HbBool isAssumption);
#endif
HbNoReturn void HbReport_OS_CrashV(char const * const function, unsigned const line, char const * const format, va_list const arguments);
#ifdef HbReport_Build_Message
void HbReport_OS_MessageV(char const * const format, va_list const arguments);
#endif
#ifdef HbReport_Build_Profile
void HbReport_OS_Profile_Span_BeginV(uint32_t const color0xRGB, char const * const format, va_list const arguments);
void HbReport_OS_Profile_Span_End();
void HbReport_OS_Profile_MarkerV(uint32_t const color0xRGB, char const * const format, va_list const arguments);
#endif

// Not OS-specific.

// Use `condition && "message"` to attach a message to an assertion.
// Can't use HbUnreachable in builds with assertions enabled because it'd make the crash itself "unreachable",
// and additional checks may be done for the unexpected case to display more info.
#ifdef HbReport_Build_Assert
#define HbReport_Assert_Checked(condition)\
do {\
	if (!(condition)) {\
		HbReport_OS_AssertCrash(__func__, __LINE__, #condition, HbFalse);\
	}\
} while (HbFalse)
#define HbReport_Assert_Assume(condition)\
do {\
	if (!(condition)) {\
		HbReport_OS_AssertCrash(__func__, __LINE__, #condition, HbTrue);\
	}\
} while (HbFalse)
#else
#define HbReport_Assert_Checked(condition) HbNoOp
#define HbReport_Assert_Assume(condition)\
do {\
	if (!(condition)) {\
		HbUnreachable();\
	}\
} while (HbFalse)
#endif

HbNoReturn void HbReport_CrashExplicit(char const * const function, unsigned const line, char const * const format, ...);
#define HbReport_Crash(format, ...) HbReport_CrashExplicit(__func__, __LINE__, format, __VA_ARGS__)

#ifdef HbReport_Build_Message
#define HbReport_MessageV HbReport_OS_MessageV
void HbReport_Message(char const * const format, ...);
#else
#define HbReport_MessageV(format, arguments) HbNoOp
#define HbReport_Message(format, ...) HbNoOp
#endif

#ifdef HbReport_Build_Profile
#define HbReport_Profile_Span_BeginV HbReport_OS_Profile_Span_BeginV
void HbReport_Profile_Span_Begin(uint32_t const color0xRGB, char const * const format, ...);
#define HbReport_Profile_Span_End HbReport_OS_Profile_Span_End
#define HbReport_Profile_MarkerV HbReport_OS_Profile_MarkerV
void HbReport_Profile_Marker(uint32_t const color0xRGB, char const * const format, ...);
#else
#define HbReport_Profile_Span_BeginV(color0xRGB, format, arguments) HbNoOp
#define HbReport_Profile_Span_Begin(color0xRGB, format, ...) HbNoOp
#define HbReport_Profile_Span_End() HbNoOp
#define HbReport_Profile_MarkerV(color0xRGB, format, arguments) HbNoOp
#define HbReport_Profile_Marker(color0xRGB, format, ...) HbNoOp
#endif

#ifdef __cplusplus
}
#endif
#endif
