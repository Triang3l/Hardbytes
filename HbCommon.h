#ifndef HbInclude_HbCommon
#define HbInclude_HbCommon

// The deepest-level header, which needs to be included in every header that doesn't include any other engine header.

// General rule about basic types (type choice is based primarily on semantics, not just size):
// - Signed types only when values can be meaningfully negative, unsigned types in all other cases.
// - (u)int_least8/16/32/64 - when need to support a specific range, and it might be nice to tightly pack a value (in structs).
// - (u)int_fast8/16/32/64 - when need to support a specific range, and it's a local variable or something where packing is unnecessary.
// - (u)int8/16/32/64_t - when explicitly need a specific size: atomics, bit fields, serialized data.
// - size_t - data counts, indices within memory. Unless, like, some specifically uint32_t hash is used, use size_t instead of some uint32_t - 4 bytes is relatively nothing.
// - char - only for ASCII strings (explicitly cast to uint8_t when inconsistency of signedness between compilers may matter).
// - HbByte (unsigned char) - smallest addressable unit in both internal buffers and files/packets, for void * supporting sizeof/HbOffsetOf-derived offsets.
// - HbBool (HbByte) - for binary states expressed as zero and non-zero values.
// - (un)signed - rarely, for abstract values when their range is not strictly defined, but more or less small, mostly for external interfaces. No need to append "int".
// - char for numbers or bytes, short, long, long long - never, unless required by an external interface!
// - wchar_t - never! Only Windows, where WCHAR (without wchar.h) can be used for internal strings, or reinterpreting can be done between WCHAR and the engine's UTF-16 text.
// Struct/union member suffixes:
// - _r - can be read directly externally, but not set without a proper setter (mutex locking may also be required).
// - _i - internal to the subsystem the structure is a part of. May also apply to entire types when all references to them are _i.

#include <stdarg.h>
#include <stdint.h> // Integers of specific size and their limits.
#include <stdlib.h> // NULL, common functions like abs.
#include <string.h> // Memory operations.

// Nothing macro.
#define HbNoOp ((void) 0)

// Compiler.
#if defined(_MSC_VER)
#define HbPlatform_Compiler_VisualC
#else
#error HbPlatform_Compiler: Unsupported compiler.
#endif

// CPU architecture. Assuming data is and will always be little-endian.
#if defined(_M_AMD64) || defined(__x86_64__)
#define HbPlatform_CPU_x86
#define HbPlatform_CPU_x86_64Bit
#elif defined(_M_IX86) || defined(__i386__)
#define HbPlatform_CPU_x86
#define HbPlatform_CPU_x86_32Bit
#elif defined(_M_ARM64) || defined(__aarch64__)
#define HbPlatform_CPU_Arm
#define HbPlatform_CPU_Arm_64Bit
#elif defined(_M_ARM) || defined(__arm__)
#define HbPlatform_CPU_Arm
#define HbPlatform_CPU_Arm_32Bit
#else
#error HbPlatform_CPU: Unsupported target CPU.
#endif
#if defined(HbPlatform_CPU_x86_32Bit) || defined(HbPlatform_CPU_Arm_32Bit)
#define HbPlatform_CPU_Bits 32
#else
#define HbPlatform_CPU_Bits 64
#endif
#if defined(HbPlatform_CPU_x86) && defined(__AVX__)
#define HbPlatform_CPU_x86_AVX // On consoles - otherwise targeting SSE3.
#endif

// Allocation alignment (at least the size of the largest scalar).
// For choosing the correct alignment of vector loads/stores primarily.
// Bigger alignment should be done manually.
#if HbPlatform_CPU_Bits < 64
#define HbPlatform_AllocAlignment 8
#else
#define HbPlatform_AllocAlignment 16
#endif

// Operating system.
#if defined(_WIN32)
#define HbPlatform_OS_Microsoft
#define HbPlatform_OS_Microsoft_Windows
// Use winapifamily.h to detect desktop or app.
#else
#error HbPlatform_OS: Unsupported target OS.
#endif

// Static assertion.
#if defined(HbPlatform_Compiler_VisualC)
#define HbStaticAssert static_assert
#else
#error HbStaticAssert: No implementation for the current compiler.
#endif

// Argument not used in the implementation.
#define HbUnused(argument) (void) (argument)

// Unreachable branch, for assertions.
#if defined(HbPlatform_Compiler_VisualC)
#define HbUnreachable() __assume(0)
#else
#error HbUnreachable: No implementation for the current compiler.
#endif

// Shorter name for the smallest addressable unit.
typedef unsigned char HbByte;

// Alignment of entities - place HbAligned after the struct keyword.
#if defined(HbPlatform_Compiler_VisualC)
#define HbAligned(alignment) __declspec(align(alignment))
#else
#error HbAligned: No implementation for the current compiler.
#endif

// Array size.
#define HbCountOf(array) (sizeof(array) / sizeof((array)[0]))

// Field offset.
#define HbOffsetOf(type, field) ((size_t) (HbByte const *) &(((type *) NULL)->field))

// Force inlining.
#if defined(HbPlatform_Compiler_VisualC)
#define HbForceInline __forceinline
#else
#error HbForceInline: No implementation for the current compiler.
#endif

// Functions leading to termination.
#if defined(HbPlatform_Compiler_VisualC)
#define HbNoReturn __declspec(noreturn)
#else
#error HbNoReturn: No implementation for the current compiler.
#endif

// Stack allocation - don't call with 0, it may cause freeing of all the allocated memory on some compilers.
#if defined(HbPlatform_OS_Microsoft)
#include <malloc.h>
#define HbStackAlloc(type, count) ((type *) _alloca((count) * sizeof(type)))
#else
#include <alloca.h>
#define HbStackAlloc(type, count) ((type *) alloca((count) * sizeof(type)))
#endif

// Boolean with a consistent size, if C/C++ interoperability is needed.
typedef HbByte HbBool;
#define HbFalse ((HbBool) 0)
#define HbTrue ((HbBool) 1)

#ifdef __cplusplus
extern "C" {
#endif

// Byte swap.
#if defined(HbPlatform_Compiler_VisualC)
#define HbByteSwapU16 _byteswap_ushort
#define HbByteSwapU32 _byteswap_ulong
#define HbByteSwapU64 _byteswap_uint64
#else
#error HbByteSwap: No implementation for the current compiler.
#endif

#ifdef __cplusplus
}
#endif

#endif
