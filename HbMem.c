#include "HbList.h"
#include "HbMath.h"
#include "HbMem.h"
#include "HbPara.h"
#include "HbReport.h"
#include "HbText.h"

/*********************************
 * Tagged heap memory allocations
 *********************************/

void HbMem_Tag_Root_Shutdown(HbMem_Tag_Root * const tagRoot) {
	HbReport_Assert_Assume(tagRoot != NULL);
	#if defined(HbReport_Build_Assert) && defined(HbReport_Build_Message)
	HbMem_Tag * tag;
	for (tag = tagRoot->tagFirst_r; tag != NULL; tag = tag->tagNext_r) {
		HbReport_Message("HbMem_Tag_Root_Shutdown: Tag %s with %zu bytes allocated not destroyed.", HbMem_Tag_GetName(tag), tag->allocationTotalSize_r);
	}
	#endif
	HbReport_Assert_Assume(tagRoot->tagFirst_r == NULL && "All allocated tags must be destroyed.");
	HbPara_Mutex_Shutdown(&tagRoot->tagListMutex_r);
}

HbMem_Tag * HbMem_Tag_Create(HbMem_Tag_Root * const tagRoot, char const * const name) {
	HbReport_Assert_Assume(tagRoot != NULL);
	size_t const nameSize = (name != NULL ? HbTextA_Length(name) : 0) + 1;
	HbMem_Tag * const tag = (HbMem_Tag *) malloc(sizeof(HbMem_Tag) + nameSize);
	if (tag == NULL) {
		HbReport_Crash("Failed to allocate memory for a memory tag.");
	}

	tag->tagRoot_r = tagRoot;
	HbPara_Mutex_Init(&tag->allocationMutex_r, HbFalse);
	tag->allocationFirst_r = tag->allocationLast_r = NULL;
	tag->allocationTotalSize_r = 0;
	HbTextA_Copy((char *) (tag + 1), nameSize, 0, name != NULL ? name : "");

	HbPara_Mutex_Lock(&tagRoot->tagListMutex_r);
	HbList_2Way_Append(tag, tagRoot->tagFirst_r, tagRoot->tagLast_r, tagPrev_r, tagNext_r);
	HbPara_Mutex_Unlock(&tagRoot->tagListMutex_r);

	return tag;
}

void HbMem_Tag_Destroy(HbMem_Tag * const tag) {
	HbReport_Assert_Assume(tag != NULL);

	#if defined(HbReport_Build_Assert) && defined(HbReport_Build_Message)
	HbMem_Tag_Allocation * allocation;
	for (allocation = tag->allocationFirst_r; allocation != NULL; allocation = allocation->tagAllocationNext_r) {
		HbReport_Message("HbMem_Tag_Destroy: Tag %s has allocation of %zu bytes at %s:%u not freed.",
		                 HbMem_Tag_GetName(tag), allocation->size_r, allocation->originNameImmutable_r, allocation->originLocation_r);
	}
	#endif
	HbReport_Assert_Assume(tag->allocationFirst_r == NULL);

	HbMem_Tag_Root * const tagRoot = tag->tagRoot_r;
	HbPara_Mutex_Lock(&tagRoot->tagListMutex_r);
	HbList_2Way_Unlink(tag, tagRoot->tagFirst_r, tagRoot->tagLast_r, tagPrev_r, tagNext_r);
	HbPara_Mutex_Unlock(&tagRoot->tagListMutex_r);

	HbPara_Mutex_Shutdown(&tag->allocationMutex_r);
	free(tag);
}

void * HbMem_Tag_AllocExplicit(HbMem_Tag * const tag, size_t const size, HbBool const required, char const * const originNameImmutable, unsigned const originLocation) {
	HbReport_Assert_Assume(tag != NULL);
	HbMem_Tag_Allocation * const allocation = (HbMem_Tag_Allocation *) malloc(sizeof(HbMem_Tag_Allocation) + size);
	if (allocation == NULL) {
		if (required) {
			HbReport_Crash("Failed to allocate %zu bytes at %s:%u with tag %s.",
			               size, originNameImmutable != NULL ? originNameImmutable : "", originLocation, HbMem_Tag_GetName(tag));
		}
		return NULL;
	}

	allocation->tag_r = tag;
	allocation->size_r = size;
	allocation->originNameImmutable_r = originNameImmutable != NULL ? originNameImmutable : "";
	allocation->originLocation_r = originLocation;

	HbPara_Mutex_Lock(&tag->allocationMutex_r);
	HbList_2Way_Append(allocation, tag->allocationFirst_r, tag->allocationLast_r, tagAllocationPrev_r, tagAllocationNext_r);
	tag->allocationTotalSize_r += size;
	HbPara_Mutex_Unlock(&tag->allocationMutex_r);

	return allocation + 1;
}

void * HbMem_Tag_AllocElementsExplicit(HbMem_Tag * const tag, size_t const elementSize, size_t count, HbBool const required,
                                       char const * const originNameImmutable, unsigned const originLocation) {
	HbReport_Assert_Assume(tag != NULL);
	HbReport_Assert_Assume(elementSize != 0);
	#ifdef HbMem_SizeMaxChecksNeeded
	size_t const maxCount = SIZE_MAX / elementSize;
	if (count > maxCount) {
		if (required) {
			HbReport_Crash("Too many %zu-sized elements (%zu, max %zu) requested at %s:%u for allocation with tag %s.",
			               elementSize, count, maxCount, originNameImmutable != NULL ? originNameImmutable : "", originLocation, HbMem_Tag_GetName(tag));
		}
		return NULL;
	}
	#endif
	return HbMem_Tag_AllocExplicit(tag, elementSize * count, required, originNameImmutable, originLocation);
}

HbBool HbMem_Tag_ReallocExplicit(void * * const buffer, size_t const size, HbBool const required) {
	HbReport_Assert_Assume(buffer != NULL);
	HbReport_Assert_Assume(*buffer != NULL);
	HbMem_Tag_Allocation * const allocation = (HbMem_Tag_Allocation *) *buffer - 1;
	HbMem_Tag * const tag = allocation->tag_r;

	// Remove the allocation from the list not to hold the mutex during the allocation because the element's address may change.
	HbPara_Mutex_Lock(&tag->allocationMutex_r);
	HbList_2Way_Unlink(allocation, tag->allocationFirst_r, tag->allocationLast_r, tagAllocationPrev_r, tagAllocationNext_r);
	tag->allocationTotalSize_r -= allocation->size_r;
	HbPara_Mutex_Unlock(&tag->allocationMutex_r);

	HbMem_Tag_Allocation * const newAllocation = (HbMem_Tag_Allocation *) realloc(allocation, sizeof(HbMem_Tag_Allocation) + size);
	if (newAllocation == NULL) {
		if (required) {
			HbReport_Crash("Failed to reallocate %zu -> %zu bytes originally allocated at %s:%u with tag %s.",
			               allocation->size_r, size, allocation->originNameImmutable_r, allocation->originLocation_r, HbMem_Tag_GetName(tag));
		}
		HbPara_Mutex_Lock(&tag->allocationMutex_r);
		HbList_2Way_Append(allocation, tag->allocationFirst_r, tag->allocationLast_r, tagAllocationPrev_r, tagAllocationNext_r);
		tag->allocationTotalSize_r += allocation->size_r;
		HbPara_Mutex_Unlock(&tag->allocationMutex_r);
		return HbFalse;
	}
	newAllocation->size_r = size;

	HbPara_Mutex_Lock(&tag->allocationMutex_r);
	HbList_2Way_Append(newAllocation, tag->allocationFirst_r, tag->allocationLast_r, tagAllocationPrev_r, tagAllocationNext_r);
	tag->allocationTotalSize_r += size;
	HbPara_Mutex_Unlock(&tag->allocationMutex_r);

	*buffer = newAllocation + 1;
	return HbTrue;
}

HbBool HbMem_Tag_ReallocElementsExplicit(void * * const buffer, size_t const elementSize, size_t count, HbBool const required) {
	HbReport_Assert_Assume(buffer != NULL);
	HbReport_Assert_Assume(*buffer != NULL);
	HbReport_Assert_Assume(elementSize != 0);
	#ifdef HbMem_SizeMaxChecksNeeded
	size_t const maxCount = SIZE_MAX / elementSize;
	if (count > maxCount) {
		if (required) {
			HbMem_Tag_Allocation * const allocation = (HbMem_Tag_Allocation *) *buffer - 1;
			HbReport_Crash("Too many %zu-sized elements (%zu, max %zu) requested for reallocation of %zu bytes originally allocated at %s:%u with tag %s.",
			               elementSize, count, maxCount, allocation->size_r,
			               allocation->originNameImmutable_r, allocation->originLocation_r, HbMem_Tag_GetName(allocation->tag_r));
		}
		return HbFalse;
	}
	#endif
	return HbMem_Tag_ReallocExplicit(buffer, elementSize * count, required);
}

void HbMem_Tag_Free(void * const buffer) {
	HbReport_Assert_Assume(buffer != NULL);
	HbMem_Tag_Allocation * const allocation = (HbMem_Tag_Allocation *) buffer - 1;

	HbMem_Tag * const tag = allocation->tag_r;
	HbPara_Mutex_Lock(&tag->allocationMutex_r);
	HbList_2Way_Unlink(allocation, tag->allocationFirst_r, tag->allocationLast_r, tagAllocationPrev_r, tagAllocationNext_r);
	tag->allocationTotalSize_r -= allocation->size_r;
	HbPara_Mutex_Unlock(&tag->allocationMutex_r);

	free(allocation);
}

/***********************
 * Dynamic-length array
 ***********************/

size_t HbMem_DynArray_GetCapacityForGrowingExplicit(size_t const elementSize, size_t const currentCapacity, size_t const neededSize) {
	HbReport_Assert_Assume(elementSize != 0);
	if (neededSize <= currentCapacity) {
		return currentCapacity;
	}
	#ifdef HbMem_SizeMaxChecksNeeded
	size_t const maxCapacity = SIZE_MAX / elementSize;
	if (maxCapacity == 0) {
		return 0;
	}
	if (neededSize >= maxCapacity) {
		return maxCapacity;
	}
	#endif
	// Grow by a factor of 1.5 (2 doesn't theoretically allow reuse of allocated memory).
	size_t const addition = neededSize / 2;
	#ifdef HbMem_SizeMaxChecksNeeded
	if (maxCapacity - neededSize < addition) {
		return neededSize;
	}
	#endif
	// Don't do many reallocations in the beginning if elements are tiny.
	return HbMath_MaxSize(neededSize + addition, HbMem_Tag_RecommendedMinAlloc / elementSize);
}

void HbMem_DynArray_ReserveExactly(HbMem_DynArray * const array, size_t const capacity, HbBool const trim) {
	HbReport_Assert_Assume(array != NULL);
	size_t const neededCapacity = HbMath_MaxSize(capacity, array->count_r);
	if (neededCapacity == array->capacity_r || (!trim && neededCapacity < array->capacity_r)) {
		return;
	}
	#ifdef HbMem_SizeMaxChecksNeeded
	size_t const maxCapacity = SIZE_MAX / array->elementSize_r;
	HbReport_Assert_Checked(neededCapacity <= maxCapacity);
	if (neededCapacity > maxCapacity) {
		HbReport_Crash("Too many elements of size %zu requested (%zu, max %zu) for the array created at %s:%u.",
		               array->elementSize_r, neededCapacity, maxCapacity, array->originNameImmutable_r, array->originLocation_r);
	}
	#endif
	if (array->capacity_r != 0) {
		if (neededCapacity == 0) {
			HbMem_Tag_Free(array->data_r);
		} else {
			HbMem_Tag_ReallocElementsExplicit((void * *) &array->data_r, array->elementSize_r, neededCapacity, HbTrue);
		}
	} else {
		array->data_r = HbMem_Tag_AllocElementsExplicit(array->tag_r, array->elementSize_r, neededCapacity, HbTrue,
		                                                array->originNameImmutable_r, array->originLocation_r);
	}
	array->capacity_r = neededCapacity;
}
