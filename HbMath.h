#ifndef HbInclude_HbMath
#define HbInclude_HbMath
#include "HbCommon.h"
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif

// In general, server and shared game logic must be fully deterministic across targets compatible with IEEE 754.
// Thus, when writing anything but some purely client-side code like rendering, some rules must be followed:
// - The code must be compiled with unsafe math optimizations disabled.
// - Most operations must be derived from unfused multiplication and addition since those are precise.
// - Library functions are only acceptable if precision is guaranteed across targets.
// - If a library function or a CPU instruction has target-dependent precision, it must be rewritten manually.
// - If there's a faster non-deterministic implementation of an operation, it must be explicitly marked as such (NonDetF32).
// - Scalar and vector versions of the same operation must give exactly the same result.
//   - When there's a custom vector implementation of a library function, an equivalent scalar implementation must be provided.
// - Decimal floating-point literals are not allowed, only hexadecimal and reinterpretation as integer are.
// - Floating-point values stored in text files follow the same rules - use %a formatting instead of %f.

// Aligning values.
#define HbMath_Align(value, alignment) (((value) + ((alignment) - 1u)) & ~((alignment) - 1u))
HbForceInline unsigned HbMath_AlignU(unsigned const value, unsigned const alignment) {
	unsigned const mask = alignment - 1;
	return (value + mask) & ~mask;
}
HbForceInline size_t HbMath_AlignSize(size_t const value, size_t const alignment) {
	size_t const mask = alignment - 1;
	return (value + mask) & ~mask;
}

// Min/max - for runtime floating-point, use fmin/fmax because of NaN handling and minss/maxss.
// The generic versions are for compile-time constants primarily because they evaluate arguments multiple times.
#define HbMath_Min(a, b) ((a) < (b) ? (a) : (b))
HbForceInline unsigned HbMath_MinU(unsigned const a, unsigned const b) { return HbMath_Min(a, b); }
HbForceInline signed HbMath_MinS(signed const a, signed const b) { return HbMath_Min(a, b); }
HbForceInline size_t HbMath_MinSize(size_t const a, size_t const b) { return HbMath_Min(a, b); }
#define HbMath_Max(a, b) ((a) > (b) ? (a) : (b))
HbForceInline unsigned HbMath_MaxU(unsigned const a, unsigned const b) { return HbMath_Max(a, b); }
HbForceInline signed HbMath_MaxS(signed const a, signed const b) { return HbMath_Max(a, b); }
HbForceInline size_t HbMath_MaxSize(size_t const a, size_t const b) { return HbMath_Max(a, b); }
#define HbMath_Clamp(value, low, high) (((value) > (high)) ? (high) : (((value) < (low)) ? (low) : (value)))
HbForceInline unsigned HbMath_ClampU(unsigned const value, unsigned const low, unsigned const high) { return HbMath_Clamp(value, low, high); }
HbForceInline signed HbMath_ClampS(signed const value, signed const low, signed const high) { return HbMath_Clamp(value, low, high); }
HbForceInline size_t HbMath_ClampSize(size_t const value, size_t const low, size_t const high) { return HbMath_Clamp(value, low, high); }
#define HbMath_ClampF32(value, low, high) fminf(high, fmaxf(low, value))

#ifdef __cplusplus
}
#endif
#endif
