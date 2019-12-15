#include "HbList.h"
#include "HbMem.h"
#include "HbPara.h"
#include "HbReport.h"
#include "HbText.h"

void HbMem_Tag_Root_Shutdown(HbMem_Tag_Root * const tagRoot) {
	HbReport_Assert_Assume(tagRoot != NULL);
	#if defined(HbReport_Build_Assert) && defined(HbReport_Build_Message)
	HbMem_Tag * tag;
	for (tag = tagRoot->tagFirst; tag != NULL; tag = tag->tagNext) {
		HbReport_Message("HbMem_Tag_Root_Shutdown: Tag %s with %zu bytes allocated not destroyed.", HbMem_Tag_GetName(tag), tag->allocationTotalSize);
	}
	#endif
	HbReport_Assert_Assume(tagRoot->tagFirst == NULL && "All allocated tags must be destroyed.");
	HbPara_Mutex_Shutdown(&tagRoot->tagListMutex);
}

HbMem_Tag * HbMem_Tag_Create(HbMem_Tag_Root * const tagRoot, char const * const name) {
	HbReport_Assert_Assume(tagRoot != NULL);
	size_t const nameSize = (name != NULL ? HbTextA_Length(name) : 0) + 1;
	HbMem_Tag * const tag = (HbMem_Tag *) malloc(sizeof(HbMem_Tag) + nameSize);
	if (tag == NULL) {
		HbReport_Crash("Failed to allocate memory for a memory tag.");
	}

	tag->tagRoot = tagRoot;
	HbPara_Mutex_Init(&tag->allocationMutex, HbFalse);
	tag->allocationFirst = tag->allocationLast = NULL;
	tag->allocationTotalSize = 0;
	HbTextA_Copy((char *) (tag + 1), nameSize, 0, name != NULL ? name : "");

	HbPara_Mutex_Lock(&tagRoot->tagListMutex);
	HbList_2Way_Append(tag, tagRoot->tagFirst, tagRoot->tagLast, tagPrev, tagNext);
	HbPara_Mutex_Unlock(&tagRoot->tagListMutex);

	return tag;
}

void HbMem_Tag_Destroy(HbMem_Tag * const tag) {
	HbReport_Assert_Assume(tag != NULL);

	#if defined(HbReport_Build_Assert) && defined(HbReport_Build_Message)
	HbMem_Tag_Allocation * allocation;
	for (allocation = tag->allocationFirst; allocation != NULL; allocation = allocation->tagAllocationNext) {
		HbReport_Message("HbMem_Tag_Destroy: Tag %s has allocation of %zu bytes at %s:%u not freed.",
		                 HbMem_Tag_GetName(tag), allocation->size, allocation->originNameImmutable, allocation->originLocation);
	}
	#endif
	HbReport_Assert_Assume(tag->allocationFirst == NULL);

	HbMem_Tag_Root * const tagRoot = tag->tagRoot;
	HbPara_Mutex_Lock(&tagRoot->tagListMutex);
	HbList_2Way_Unlink(tag, tagRoot->tagFirst, tagRoot->tagLast, tagPrev, tagNext);
	HbPara_Mutex_Unlock(&tagRoot->tagListMutex);

	HbPara_Mutex_Shutdown(&tag->allocationMutex);
	free(tag);
}

void * HbMem_Tag_AllocExplicit(HbMem_Tag * const tag, size_t const size, HbBool const required, char const * const originNameImmutable, unsigned const originLocation) {
	HbReport_Assert_Assume(tag != NULL);
	HbReport_Assert_Assume(originNameImmutable != NULL);
	HbMem_Tag_Allocation * const allocation = (HbMem_Tag_Allocation *) malloc(sizeof(HbMem_Tag_Allocation) + size);
	if (allocation == NULL) {
		if (required) {
			HbReport_Crash("Failed to allocate %zu bytes at %s:%u with tag %s.", size, originNameImmutable, originLocation, HbMem_Tag_GetName(tag));
		}
		return NULL;
	}

	allocation->tag = tag;
	allocation->size = size;
	allocation->originNameImmutable = originNameImmutable;
	allocation->originLocation = originLocation;

	HbPara_Mutex_Lock(&tag->allocationMutex);
	HbList_2Way_Append(allocation, tag->allocationFirst, tag->allocationLast, tagAllocationPrev, tagAllocationNext);
	tag->allocationTotalSize += size;
	HbPara_Mutex_Unlock(&tag->allocationMutex);

	return allocation + 1;
}

HbBool HbMem_Tag_ReallocExplicit(void * * const buffer, size_t const size, HbBool const required) {
	HbReport_Assert_Assume(buffer != NULL);
	HbReport_Assert_Assume(*buffer != NULL);
	HbMem_Tag_Allocation * const allocation = (HbMem_Tag_Allocation *) *buffer - 1;
	HbMem_Tag * const tag = allocation->tag;

	// Remove the allocation from the list not to hold the mutex during the allocation because the element's address may change.
	HbPara_Mutex_Lock(&tag->allocationMutex);
	HbList_2Way_Unlink(allocation, tag->allocationFirst, tag->allocationLast, tagAllocationPrev, tagAllocationNext);
	tag->allocationTotalSize -= allocation->size;
	HbPara_Mutex_Unlock(&tag->allocationMutex);

	HbMem_Tag_Allocation * const newAllocation = (HbMem_Tag_Allocation *) realloc(allocation, sizeof(HbMem_Tag_Allocation) + size);
	if (newAllocation == NULL) {
		if (required) {
			HbReport_Crash("Failed to reallocate %zu -> %zu bytes originally allocated at %s:%u with tag %s.",
			               allocation->size, size, allocation->originNameImmutable, allocation->originLocation, HbMem_Tag_GetName(tag));
		}
		HbPara_Mutex_Lock(&tag->allocationMutex);
		HbList_2Way_Append(allocation, tag->allocationFirst, tag->allocationLast, tagAllocationPrev, tagAllocationNext);
		tag->allocationTotalSize += allocation->size;
		HbPara_Mutex_Unlock(&tag->allocationMutex);
		return HbFalse;
	}
	newAllocation->size = size;

	HbPara_Mutex_Lock(&tag->allocationMutex);
	HbList_2Way_Append(newAllocation, tag->allocationFirst, tag->allocationLast, tagAllocationPrev, tagAllocationNext);
	tag->allocationTotalSize += size;
	HbPara_Mutex_Unlock(&tag->allocationMutex);

	*buffer = newAllocation + 1;
	return HbTrue;
}

size_t HbMem_Tag_GetAllocSize(void const * const buffer) {
	HbReport_Assert_Assume(buffer != NULL);
	return ((HbMem_Tag_Allocation const *) buffer - 1)->size;
}

void HbMem_Tag_Free(void * const buffer) {
	HbReport_Assert_Assume(buffer != NULL);
	HbMem_Tag_Allocation * const allocation = (HbMem_Tag_Allocation *) buffer - 1;

	HbMem_Tag * const tag = allocation->tag;
	HbPara_Mutex_Lock(&tag->allocationMutex);
	HbList_2Way_Unlink(allocation, tag->allocationFirst, tag->allocationLast, tagAllocationPrev, tagAllocationNext);
	tag->allocationTotalSize -= allocation->size;
	HbPara_Mutex_Unlock(&tag->allocationMutex);

	free(allocation);
}
