#include "HbReport.h"

HbNoReturn void HbReport_CrashExplicit(char const * const function, unsigned const line, char const * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	HbReport_OS_CrashV(function, line, format, arguments);
	va_end(arguments);
}

#ifdef HbReport_Build_Message
void HbReport_Message(char const * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	HbReport_MessageV(format, arguments);
	va_end(arguments);
}
#endif

#ifdef HbReport_Build_Profile
void HbReport_Profile_Span_Begin(uint32_t const color0xRGB, char const * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	HbReport_Profile_Span_BeginV(color0xRGB, format, arguments);
	va_end(arguments);
}

void HbReport_Profile_Marker(uint32_t const color0xRGB, char const * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	HbReport_Profile_MarkerV(color0xRGB, format, arguments);
	va_end(arguments);
}
#endif
