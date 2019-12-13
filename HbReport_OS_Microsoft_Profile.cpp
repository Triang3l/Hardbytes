#include "HbReport.h"
#if defined(HbReport_Build_Profile) && defined(HbPlatform_OS_Microsoft)
#include "HbText.h"
#include <Windows.h>
#include <pix3.h>

void HbReport_OS_Profile_Span_BeginV(uint32_t const color0xRGB, char const * const format, va_list const arguments) {
	HbReport_Assert_Assume(format != NULL);
	size_t const nameBufferSize = HbTextA_FormatLengthV(format, arguments) + 1;
	char * const name = HbStackAlloc(char, nameBufferSize);
	HbTextA_FormatV(name, nameBufferSize, 0, format, arguments);
	PIXBeginEvent(PIX_COLOR((color0xRGB >> 16) & 0xFF, (color0xRGB >> 8) & 0xFF, color0xRGB & 0xFF), "%s", name);
}

void HbReport_OS_Profile_Span_End() {
	PIXEndEvent();
}

void HbReport_OS_Profile_MarkerV(uint32_t const color0xRGB, char const * const format, va_list const arguments) {
	HbReport_Assert_Assume(format != NULL);
	size_t const nameBufferSize = HbTextA_FormatLengthV(format, arguments) + 1;
	char * const name = HbStackAlloc(char, nameBufferSize);
	HbTextA_FormatV(name, nameBufferSize, 0, format, arguments);
	PIXSetMarker(PIX_COLOR((color0xRGB >> 16) & 0xFF, (color0xRGB >> 8) & 0xFF, color0xRGB & 0xFF), "%s", name);
}

#endif
