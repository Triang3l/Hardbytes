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
	struct HbMem_Tag * tag_r;
	struct HbMem_Tag_Allocation * tagAllocationPrev_r; // Lock tag_r->allocationMutex_r.
	struct HbMem_Tag_Allocation * tagAllocationNext_r; // Lock tag_r->allocationMutex_r.
	size_t size_r;
	char const * originNameImmutable_r; // Function name generally, but can be something else (like, a library only providing file names).
	unsigned originLocation_r; // File line generally.
} HbMem_Tag_Allocation;

typedef struct HbMem_Tag {
	HbMem_Tag_Root * tagRoot_r;
	struct HbMem_Tag * tagPrev_r; // Lock tagRoot_r->tagListMutex_r.
	struct HbMem_Tag * tagNext_r; // Lock tagRoot_r->tagListMutex_r.
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
#define HbMem_Tag_Alloc(tag, type, count) ((type *) HbMem_Tag_AllocExplicit((tag), (count) * sizeof(type), HbTrue, __func__, __LINE__))
#define HbMem_Tag_AllocChecked(tag, type, count) ((type *) HbMem_Tag_AllocExplicit((tag), (count) * sizeof(type), HbFalse, __func__, __LINE__))
// The buffer must already be allocated with Alloc because no origin info is passed to this and so intentions need to be specified clearly to reduce error probability.
HbBool HbMem_Tag_ReallocExplicit(void * * const buffer, size_t const size, HbBool const required);
#define HbMem_Tag_Realloc(buffer, type, count) HbMem_Tag_ReallocExplicit((void * *) &(buffer), (count) * sizeof(type), HbTrue)
#define HbMem_Tag_ReallocChecked(buffer, type, count) HbMem_Tag_ReallocExplicit((void * *) &(buffer), (count) * sizeof(type), HbFalse)
// The buffer must exist - null pointers are generally not allowed to detect errors easier, and this is not an exception.
void HbMem_Tag_Free(void * const buffer);

HbForceInline HbMem_Tag_Allocation const * HbMem_Tag_GetAllocation(void const * const buffer) {
	HbReport_Assert_Assume(buffer != NULL);
	return (HbMem_Tag_Allocation const *) buffer - 1;
}

#ifdef __cplusplus
}
#endif
#endif
