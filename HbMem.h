#ifndef HbInclude_HbMem
#define HbInclude_HbMem
#include "HbPara.h"
#include "HbReport.h"
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************
 * Tagged heap memory allocations
 * For leak detection and memory usage tracking
 ***********************************************/

typedef struct HbMem_Tag_Root {
	HbPara_Mutex tagListMutex;
	struct HbMem_Tag * tagFirst;
	struct HbMem_Tag * tagLast;
} HbMem_Tag_Root;
inline void HbMem_Tag_Root_Init(HbMem_Tag_Root * const tagRoot) {
	HbReport_Assert_Assume(tagRoot != NULL);
	HbPara_Mutex_Init(&tagRoot->tagListMutex, HbFalse);
	tagRoot->tagFirst = tagRoot->tagLast = NULL;
}
void HbMem_Tag_Root_Shutdown(HbMem_Tag_Root * const tagRoot);

typedef struct HbAligned(HbPlatform_AllocAlignment) HbMem_Tag_Allocation {
	struct HbMem_Tag * tag;
	struct HbMem_Tag_Allocation * tagAllocationPrev;
	struct HbMem_Tag_Allocation * tagAllocationNext;
	size_t size;
	char const * originNameImmutable; // Function name generally, but can be something else (like, a library only providing file names).
	unsigned originLocation; // File line generally.
} HbMem_Tag_Allocation;

typedef struct HbMem_Tag {
	HbMem_Tag_Root * tagRoot;
	struct HbMem_Tag * tagPrev;
	struct HbMem_Tag * tagNext;
	HbPara_Mutex allocationMutex;
	HbMem_Tag_Allocation * allocationFirst;
	HbMem_Tag_Allocation * allocationLast;
	size_t allocationTotalSize;
	// Followed by char name[].
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
#define HbMem_Tag_Alloc(tag, type, count) ((type *) HbMem_Tag_AllocExplicit((tag), (count) * sizeof(type), HbTrue, __func__, __LINE__))
#define HbMem_Tag_AllocChecked(tag, type, count) ((type *) HbMem_Tag_AllocExplicit((tag), (count) * sizeof(type), HbFalse, __func__, __LINE__))
// The buffer must already be allocated with Alloc because no origin info is passed to this and so intentions need to be specified clearly to reduce error probability.
HbBool HbMem_Tag_ReallocExplicit(void * * const buffer, size_t const size, HbBool const required);
#define HbMem_Tag_Realloc(buffer, type, count) HbMem_Tag_ReallocExplicit((void * *) &(buffer), (count) * sizeof(type), HbTrue)
#define HbMem_Tag_ReallocChecked(buffer, type, count) HbMem_Tag_ReallocExplicit((void * *) &(buffer), (count) * sizeof(type), HbFalse)
size_t HbMem_Tag_GetAllocSize(void const * const buffer);
// The buffer must exist - null pointers are generally not allowed to detect errors easier, and this is not an exception.
void HbMem_Tag_Free(void * const buffer);

#ifdef __cplusplus
}
#endif
#endif
