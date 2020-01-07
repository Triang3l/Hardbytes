#ifndef HbInclude_HbMem
#define HbInclude_HbMem
#include "HbMath.h"
#include "HbPara.h"
#include "HbReport.h"
#ifdef __cplusplus
extern "C" {
#endif

// Whether SIZE_MAX overflow checks make sense (unlikely to use anything in the order of exabytes any time soon) for counts of items in data structures used internally.
// For external sources like user-generated content and network packets, strict checks are needed regardless of this.
#if defined(HbReport_Build_Assert) || HbPlatform_CPU_Bits < 64
#define HbMem_SizeMaxChecksNeeded
#endif

/***********************************************
 * Tagged heap memory allocations
 * For leak detection and memory usage tracking
 ***********************************************/

typedef struct HbMem_Tag_Root {
	HbPara_Mutex tagListMutex_r;
	struct HbMem_Tag * tagFirst_r; // Lock tagListMutex_r.
	struct HbMem_Tag * tagLast_r; // Lock tagListMutex_r.
} HbMem_Tag_Root;
inline void HbMem_Tag_Root_Init(HbMem_Tag_Root * const tagRoot) {
	HbReport_Assert_Assume(tagRoot != NULL);
	HbPara_Mutex_Init(&tagRoot->tagListMutex_r, HbFalse);
	tagRoot->tagFirst_r = tagRoot->tagLast_r = NULL;
}
void HbMem_Tag_Root_Shutdown(HbMem_Tag_Root * const tagRoot);

typedef struct HbAligned(HbPlatform_AllocAlignment) HbMem_Tag_Allocation {
	struct HbMem_Tag * tag_e;
	struct HbMem_Tag_Allocation * tagAllocationPrev_r; // Lock tag_e->allocationMutex_r.
	struct HbMem_Tag_Allocation * tagAllocationNext_r; // Lock tag_e->allocationMutex_r.
	size_t size_r;
	char const * originNameImmutable_r; // Function name generally, but can be something else (like, a library only providing file names).
	unsigned originLocation_r; // File line generally.
} HbMem_Tag_Allocation;

typedef struct HbMem_Tag {
	HbMem_Tag_Root * tagRoot_e;
	struct HbMem_Tag * tagPrev_r; // Lock tagRoot_e->tagListMutex_r.
	struct HbMem_Tag * tagNext_r; // Lock tagRoot_e->tagListMutex_r.
	HbPara_Mutex allocationMutex_r;
	HbMem_Tag_Allocation * allocationFirst_r; // Lock allocationMutex_r.
	HbMem_Tag_Allocation * allocationLast_r; // Lock allocationMutex_r.
	size_t allocationTotalSize_r; // Lock allocationMutex_r.
	// Followed by char name_r[].
} HbMem_Tag;
// Create instead of Init so tags themselves aren't (accidentally) created in tagged memory (and deallocated).
HbMem_Tag * HbMem_Tag_Create(HbMem_Tag_Root * const tagRoot, char const * const name);
void HbMem_Tag_Destroy(HbMem_Tag * const tag);
HbForceInline char const * HbMem_Tag_GetName(HbMem_Tag const * const tag) {
	HbReport_Assert_Assume(tag != NULL);
	return (char const *) (tag + 1);
}

// The returned buffer has alignment of HbPlatform_AllocAlignment - no built-in alignment handling to avoid overcomplicating realloc.
void * HbMem_Tag_AllocExplicit(HbMem_Tag * const tag, size_t const size, HbBool const required, char const * const originNameImmutable, unsigned const originLocation);
void * HbMem_Tag_AllocElementsExplicit(HbMem_Tag * const tag, size_t const elementSize, size_t count, HbBool const required,
                                       char const * const originNameImmutable, unsigned const originLocation);
#define HbMem_Tag_Alloc(tag, type, count) ((type *) HbMem_Tag_AllocElementsExplicit(tag, sizeof(type), count, HbTrue, __func__, __LINE__))
#define HbMem_Tag_AllocChecked(tag, type, count) ((type *) HbMem_Tag_AllocElementsExplicit(tag, sizeof(type), count, HbFalse, __func__, __LINE__))
// The buffer must already be allocated with Alloc because no origin info is passed to this and so intentions need to be specified clearly to reduce error probability.
HbBool HbMem_Tag_ReallocExplicit(void * * const buffer, size_t const size, HbBool const required);
HbBool HbMem_Tag_ReallocElementsExplicit(void * * const buffer, size_t const elementSize, size_t count, HbBool const required);
#define HbMem_Tag_Realloc(buffer, type, count) HbMem_Tag_ReallocElementsExplicit((void * *) &(buffer), sizeof(type) * (count), HbTrue)
#define HbMem_Tag_ReallocChecked(buffer, type, count) HbMem_Tag_ReallocElementsExplicit((void * *) &(buffer), sizeof(type) * (count), HbFalse)
// The buffer must exist - null pointers are generally not allowed to detect errors easier, and this is not an exception.
void HbMem_Tag_Free(void * const buffer);

HbForceInline HbMem_Tag_Allocation const * HbMem_Tag_GetAllocation(void const * const buffer) {
	HbReport_Assert_Assume(buffer != NULL);
	return (HbMem_Tag_Allocation const *) buffer - 1;
}

// When the header is relatively not a waste of space - slightly bigger than HbMem_Tag_Allocation on a 64-bit target.
#define HbMem_Tag_RecommendedMinAlloc ((size_t) 64)

/***********************
 * Dynamic-length array
 ***********************/

typedef struct HbMem_DynArray {
	void * data_r;
	size_t elementSize_r;
	size_t capacity_r;
	size_t count_r;
	HbMem_Tag * tag_e;
	char const * originNameImmutable_r;
	unsigned originLocation_r;
} HbMem_DynArray;

HbForceInline void HbMem_DynArray_InitExplicit(HbMem_DynArray * const array, size_t const elementSize,
                                               HbMem_Tag * const tag, char const * const originNameImmutable, unsigned const originLocation) {
	HbReport_Assert_Assume(array != NULL);
	HbReport_Assert_Assume(elementSize != 0);
	array->data_r = NULL;
	array->elementSize_r = elementSize;
	array->capacity_r = 0;
	array->count_r = 0;
	array->tag_e = tag;
	array->originNameImmutable_r = originNameImmutable;
	array->originLocation_r = originLocation;
}
#define HbMem_DynArray_Init(array, elementType, tag) HbMem_DynArray_InitExplicit(array, sizeof(elementType), tag, __func__, __LINE__)

HbForceInline void HbMem_DynArray_Shutdown(HbMem_DynArray * const array) {
	HbReport_Assert_Assume(array != NULL);
	if (array->data_r != NULL) {
		HbMem_Tag_Free(array->data_r);
	}
}

// Pluggable in other places that behave similar to HbMem_DynArray.
size_t HbMem_DynArray_GetCapacityForGrowingExplicit(size_t const elementSize, size_t const currentCapacity, size_t const neededSize);

HbForceInline size_t HbMem_DynArray_GetCapacityForGrowing(HbMem_DynArray const * const array, size_t const neededSize) {
	HbReport_Assert_Assume(array != NULL);
	return HbMem_DynArray_GetCapacityForGrowingExplicit(array->elementSize_r, array->capacity_r, neededSize);
}

void HbMem_DynArray_ReserveExactly(HbMem_DynArray * const array, size_t const capacity, HbBool const trim);

HbForceInline void HbMem_DynArray_TrimCapacity(HbMem_DynArray * const array) {
	HbReport_Assert_Assume(array != NULL);
	HbMem_DynArray_ReserveExactly(array, array->capacity_r, HbTrue);
}

HbForceInline void HbMem_DynArray_ReserveForGrowing(HbMem_DynArray * const array, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	if (array->capacity_r >= count) {
		return;
	}
	HbMem_DynArray_ReserveExactly(array, HbMem_DynArray_GetCapacityForGrowing(array, count), HbFalse);
}

HbForceInline void HbMem_DynArray_ResizeExactly(HbMem_DynArray * const array, size_t const count, HbBool const trim) {
	HbReport_Assert_Assume(array != NULL);
	HbMem_DynArray_ReserveExactly(array, count, trim);
	array->count_r = count;
}

HbForceInline void HbMem_DynArray_ResizeForGrowing(HbMem_DynArray * const array, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	HbMem_DynArray_ReserveForGrowing(array, count);
	array->count_r = count;
}

inline void HbMem_DynArray_MakeGapInUnsorted(HbMem_DynArray * const array, size_t const offset, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	HbReport_Assert_Assume(offset <= array->count_r);
	if (array->capacity_r - array->count_r < count) {
		#ifdef HbMem_SizeMaxChecksNeeded
		size_t const countValuesLeft = SIZE_MAX / array->elementSize_r - array->count_r;
		if (countValuesLeft < count) {
			HbReport_Crash("Too many elements of size %zu requested (%zu, max %zu) for insertion into the array created at %s:%u.",
			               array->elementSize_r, count, countValuesLeft, array->originNameImmutable_r, array->originLocation_r);
		}
		#endif
		HbMem_DynArray_ReserveForGrowing(array, array->count_r + count);
	}
	size_t const countToMove = HbMath_Min_Size(count, array->count_r - offset);
	array->count_r += count;
	if (countToMove != 0) { // May even have an empty array at this point (if count is zero), and memcpy is undefined for NULL.
		memcpy((HbByte *) array->data_r + array->elementSize_r * (array->count_r - countToMove),
		       (HbByte *) array->data_r + array->elementSize_r * offset,
		       array->elementSize_r * countToMove);
	}
}

inline void HbMem_DynArray_MakeGapInSorted(HbMem_DynArray * const array, size_t const offset, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	HbReport_Assert_Assume(offset <= array->count_r);
	if (array->capacity_r - array->count_r < count) {
		#ifdef HbMem_SizeMaxChecksNeeded
		size_t const countValuesLeft = SIZE_MAX / array->elementSize_r - array->count_r;
		if (countValuesLeft < count) {
			HbReport_Crash("Too many elements of size %zu requested (%zu, max %zu) for insertion into the array created at %s:%u.",
			               array->elementSize_r, count, countValuesLeft, array->originNameImmutable_r, array->originLocation_r);
		}
		#endif
		HbMem_DynArray_ReserveForGrowing(array, array->count_r + count);
	}
	size_t const countToMove = array->count_r - offset;
	array->count_r += count;
	if (countToMove != 0) {
		memmove((HbByte *) array->data_r + array->elementSize_r * (offset + count),
		        (HbByte *) array->data_r + array->elementSize_r * offset,
		        array->elementSize_r * countToMove);
	}
}

inline size_t HbMem_DynArray_Append(HbMem_DynArray * const array, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	if (array->capacity_r - array->count_r < count) {
		#ifdef HbMem_SizeMaxChecksNeeded
		size_t const countValuesLeft = SIZE_MAX / array->elementSize_r - array->count_r;
		if (countValuesLeft < count) {
			HbReport_Crash("Too many elements of size %zu requested (%zu, max %zu) for appending to the array created at %s:%u.",
			               array->elementSize_r, count, countValuesLeft, array->originNameImmutable_r, array->originLocation_r);
		}
		#endif
		HbMem_DynArray_ReserveForGrowing(array, array->count_r + count);
	}
	return (array->count_r += count) - count;
}

inline void HbMem_DynArray_RemoveFromUnsorted(HbMem_DynArray * const array, size_t const offset, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	HbReport_Assert_Assume(offset <= array->count_r);
	HbReport_Assert_Assume(count <= array->count_r - offset);
	size_t const countToMove = HbMath_Min_Size(count, array->count_r - (offset + count));
	if (countToMove != 0) {
		memcpy((HbByte *) array->data_r + array->elementSize_r * offset,
		       (HbByte *) array->data_r + array->elementSize_r * (array->count_r - countToMove),
		       array->elementSize_r * countToMove);
	}
	array->count_r -= count;
}

inline void HbMem_DynArray_RemoveFromSorted(HbMem_DynArray * const array, size_t const offset, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	HbReport_Assert_Assume(offset <= array->count_r);
	HbReport_Assert_Assume(count <= array->count_r - offset);
	size_t const countToMove = array->count_r - (offset + count);
	if (countToMove != 0) {
		memmove((HbByte *) array->data_r + array->elementSize_r * offset,
		        (HbByte *) array->data_r + array->elementSize_r * (offset + count),
		        array->elementSize_r * countToMove);
	}
	array->count_r -= count;
}

HbForceInline void HbMem_DynArray_RemoveFromEnd(HbMem_DynArray * const array, size_t const count) {
	HbReport_Assert_Assume(array != NULL);
	HbReport_Assert_Assume(count <= array->count_r);
	array->count_r -= count;
}

HbForceInline void const * HbMem_DynArray_GetExplicit(HbMem_DynArray const * const array, size_t const index) {
	HbReport_Assert_Assume(array != NULL && "Dynamic-length array must not be NULL. Additionally, this is triggered when GetC is called with a mismatching type size.");
	HbReport_Assert_Assume(index < array->count_r);
	return (HbByte const *) array->data_r + array->elementSize_r * index;
}

HbForceInline void * HbMem_DynArray_GetMutExplicit(HbMem_DynArray * const array, size_t const index) {
	HbReport_Assert_Assume(array != NULL && "Dynamic-length array must not be NULL. Additionally, this is triggered when Get is called with a mismatching type size.");
	HbReport_Assert_Assume(index < array->count_r);
	return (HbByte *) array->data_r + array->elementSize_r * index;
}

// Compile-time immediate for the size instead of memory access when assertions are disabled.
// When assertions are enabled, the size is also checked.
// Returning a pointer to the element.
#ifdef HbReport_Build_Assert
#define HbMem_DynArray_Get(array, index, elementType) ((elementType const *) HbMem_DynArray_GetExplicit((array) != NULL && (array)->elementSize_r == sizeof(elementType) ? (array) : NULL, index))
#define HbMem_DynArray_GetMut(array, index, elementType) ((elementType *) HbMem_DynArray_GetMutExplicit((array) != NULL && (array)->elementSize_r == sizeof(elementType) ? (array) : NULL, index))
#else
#define HbMem_DynArray_Get(array, index, elementType) (((elementType const *) (array)->data_r)[index])
#define HbMem_DynArray_GetMut(array, index, elementType) (((elementType *) (array)->data_r)[index])
#endif

/***********************************************
 * Fibonacci number-sized block buddy allocator
 ***********************************************/

extern size_t const HbMem_FibAlloc_Sizes[]; // 1, 2, 3, 5... the largest not above SIZE_MAX.

size_t HbMem_FibAlloc_ClosestLevelRoundingDown(size_t const size);
size_t HbMem_FibAlloc_ClosestLevelRoundingUp(size_t const size);

// The main purpose of this allocator is allocation not of individual items, but of pools size of which may vary.
// Because of fixed allocation sizes, there's a lot of fragmentation that can't be filled later. So, allocation size is "soft".
// Instead of giving the exact amount requested, the allocator will, in the best case, return the needed amount *plus fragmentation padding*.
// If there's no free block of such size, it will try to give the largest possible block not smaller than the minimum needed size.

typedef struct HbMem_FibAlloc {
	size_t largestLevel_r;
	HbMem_DynArray /* <HbMem_FibAlloc_Node_i> */ nodes_i; // Stable indices, recycling when both children are empty. [0] is the root.
	size_t lastRecycledNodeIndex_i; // SIZE_MAX if no deallocated nodes.
	struct HbMem_FibAlloc_FreeList_i * freeLists_i; // [largestLevel_r + 1], the last element contains the whole tree as the larger child if the tree is empty.
} HbMem_FibAlloc;

void HbMem_FibAlloc_InitExplicit(HbMem_FibAlloc * const fibAlloc, size_t const largestLevel, HbMem_Tag * const tag,
                                 char const * const originNameImmutable, unsigned const originLocation);
#define HbMem_FibAlloc_Init(fibAlloc, largestLevel, tag) HbMem_FibAlloc_InitExplicit(fibAlloc, largestLevel, tag, __func__, __LINE__)
void HbMem_FibAlloc_Shutdown(HbMem_FibAlloc * const fibAlloc);

#define HbMem_FibAlloc_Alloc_Failed SIZE_MAX
// If the minimum and the preferred counts are not equal, it's required to provide a valid pointer to the output level of the size.
size_t HbMem_FibAlloc_Alloc(HbMem_FibAlloc * const fibAlloc, size_t const minimumCount, size_t const preferredCount, size_t * const allocationLevelOut);
void HbMem_FibAlloc_Free(HbMem_FibAlloc * const fibAlloc, size_t const allocation);

#ifdef __cplusplus
}
#endif
#endif
