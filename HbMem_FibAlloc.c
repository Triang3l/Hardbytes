#include "HbMath.h"
#include "HbMem.h"
#include "HbReport.h"
#include "HbSort.h"

HbStaticAssert(HbPlatform_CPU_Bits >= 32, "HbMem_FibAlloc_Sizes: Filled for 32-bit and 64-bit size_t.");
size_t const HbMem_FibAlloc_Sizes[] = {
	0x00000001,
	0x00000002,
	0x00000003,
	0x00000005,
	0x00000008,
	0x0000000D,
	0x00000015,
	0x00000022,
	0x00000037,
	0x00000059,
	0x00000090,
	0x000000E9,
	0x00000179,
	0x00000262,
	0x000003DB,
	0x0000063D,
	0x00000A18,
	0x00001055,
	0x00001A6D,
	0x00002AC2,
	0x0000452F,
	0x00006FF1,
	0x0000B520,
	0x00012511,
	0x0001DA31,
	0x0002FF42,
	0x0004D973,
	0x0007D8B5,
	0x000CB228,
	0x00148ADD,
	0x00213D05,
	0x0035C7E2,
	0x005704E7,
	0x008CCCC9,
	0x00E3D1B0,
	0x01709E79,
	0x02547029,
	0x03C50EA2,
	0x06197ECB,
	0x09DE8D6D,
	0x0FF80C38,
	0x19D699A5,
	0x29CEA5DD,
	0x43A53F82,
	0x6D73E55F,
	0xB11924E1,
	#if HbPlatform_CPU_Bits >= 64
	0x000000011E8D0A40,
	0x00000001CFA62F21,
	0x00000002EE333961,
	0x00000004BDD96882,
	0x00000007AC0CA1E3,
	0x0000000C69E60A65,
	0x0000001415F2AC48,
	0x000000207FD8B6AD,
	0x0000003495CB62F5,
	0x0000005515A419A2,
	0x00000089AB6F7C97,
	0x000000DEC1139639,
	0x000001686C8312D0,
	0x000002472D96A909,
	0x000003AF9A19BBD9,
	0x000005F6C7B064E2,
	0x000009A661CA20BB,
	0x00000F9D297A859D,
	0x000019438B44A658,
	0x000028E0B4BF2BF5,
	0x000042244003D24D,
	0x00006B04F4C2FE42,
	0x0000AD2934C6D08F,
	0x0001182E2989CED1,
	0x0001C5575E509F60,
	0x0002DD8587DA6E31,
	0x0004A2DCE62B0D91,
	0x000780626E057BC2,
	0x000C233F54308953,
	0x0013A3A1C2360515,
	0x001FC6E116668E68,
	0x00336A82D89C937D,
	0x00533163EF0321E5,
	0x00869BE6C79FB562,
	0x00D9CD4AB6A2D747,
	0x016069317E428CA9,
	0x023A367C34E563F0,
	0x039A9FADB327F099,
	0x05D4D629E80D5489,
	0x096F75D79B354522,
	0x0F444C01834299AB,
	0x18B3C1D91E77DECD,
	0x27F80DDAA1BA7878,
	0x40ABCFB3C0325745,
	0x68A3DD8E61ECCFBD,
	0xA94FAD42221F2702,
	#endif
};

size_t HbMem_FibAlloc_ClosestLevelRoundingDown(size_t const size) {
	size_t const greaterLevel = HbSort_Find_FirstGreater_Size(size, HbMem_FibAlloc_Sizes, HbCountOf(HbMem_FibAlloc_Sizes));
	return greaterLevel - (greaterLevel != 0);
}

size_t HbMem_FibAlloc_ClosestLevelRoundingUp(size_t const size) {
	return HbMath_Min_Size(HbSort_Find_FirstNotLess_Size(size, HbMem_FibAlloc_Sizes, HbCountOf(HbMem_FibAlloc_Sizes)), HbCountOf(HbMem_FibAlloc_Sizes) - 1);
}

#define HbMem_FibAlloc_Node_Child_ChildNodeIndex_Data_i 0
typedef struct HbMem_FibAlloc_Node_Child_i {
	size_t isFree_i : 1;
	// For a split child, childOrNextFreeNodeIndex_i is the index of the node with its two children.
	// For an allocation, this is set to HbMem_FibAlloc_Node_Child_ChildNodeIndex_Data_i.
	// For a free node, it's the index of the next node with an equal free child (same level, same relation to the sibling) in the free looped linked list.
	size_t childOrNextFreeNodeIndex_i : (sizeof(size_t) * CHAR_BIT - 1);
} HbMem_FibAlloc_Node_Child_i;

typedef struct HbMem_FibAlloc_Node_i {
	// Offset of the allocation address (of the smaller child).
	size_t offset_i;
	// [0] - smaller, [1] - larger.
	// Smaller nodes are on the left, except for the root node, to keep the first allocations closer to 0, since smaller nodes are preferred for splitting.
	// For node with size Fib[n], the smaller node has size Fib[n] - Fib[n - 1] because -2 is unsafe for 2, the larger node has size Fib[n - 1].
	// One exception is that on largestLevel_r of the allocator, only the "larger" child exists.
	HbMem_FibAlloc_Node_Child_i children_i[2];
	// If it's an active node with a free child, this is the previous node in the free list (see HbMem_FibAlloc_Node_Child_i::childOrNextFreeNodeIndex_i).
	// There can be at most one free child, so only one link needs to be stored.
	// If it's a recycled node, this is the node that was recycled previously, SIZE_MAX if the end.
	size_t prevFreeOrRecycledNodeIndex_i;
} HbMem_FibAlloc_Node_i;

// - HbMem_FibAlloc_Node_Child_i uses a bit for isFree_i and also reserves zero (HbMem_FibAlloc_Node_Child_ChildNodeIndex_Data_i - the root node index) for an allocation.
// - HbMem_FibAlloc_PathStep_i uses a bit for isLarger_i.
#define HbMem_FibAlloc_MaxNodes_i ((SIZE_MAX >> 1) + 1)

typedef struct HbMem_FibAlloc_FreeList_i {
	size_t freeNodeIndices_i[2]; // [0] - first smaller child on this level, [1] - first larger child on this level, SIZE_MAX if no free nodes.
} HbMem_FibAlloc_FreeList_i;

HbForceInline size_t HbMem_FibAlloc_GetChildLevel_i(size_t const parentLevel, HbBool const isLarger) {
	HbReport_Assert_Assume(parentLevel != 0);
	// Clamp because if level 1 is split, both smaller and larger children are level 0 (1-sized).
	return parentLevel - HbMath_Min_Size(isLarger ? 1 : 2, parentLevel);
}

HbForceInline size_t HbMem_FibAlloc_GetChildRelativeOffset_i(size_t const childLevel, size_t const largestLevel, HbBool const isLargerChild) {
	HbReport_Assert_Assume(childLevel <= largestLevel);
	if (!isLargerChild || childLevel >= largestLevel) {
		return 0;
	}
	return HbMem_FibAlloc_Sizes[childLevel + 1] - HbMem_FibAlloc_Sizes[childLevel]; // Larger child is placed after the smaller one.
}

static void HbMem_FibAlloc_AddNodeChildToFreeList_i(HbMem_FibAlloc * const fibAlloc, size_t const nodeIndex, HbBool const isLarger, size_t const childLevel) {
	HbReport_Assert_Assume(fibAlloc != NULL);
	HbReport_Assert_Assume(childLevel <= fibAlloc->largestLevel_r);
	HbMem_FibAlloc_Node_i * const node = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, nodeIndex, HbMem_FibAlloc_Node_i);
	node->children_i[isLarger].isFree_i = HbTrue;
	HbMem_FibAlloc_FreeList_i * const freeList = &fibAlloc->freeLists_i[childLevel];
	size_t const nextFreeNodeIndex = freeList->freeNodeIndices_i[isLarger];
	if (nextFreeNodeIndex != SIZE_MAX) {
		HbMem_FibAlloc_Node_i * const nextFreeNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, nextFreeNodeIndex, HbMem_FibAlloc_Node_i);
		HbReport_Assert_Assume(nextFreeNode->children_i[isLarger].isFree_i);
		size_t const prevFreeNodeIndex = nextFreeNode->prevFreeOrRecycledNodeIndex_i;
		HbMem_FibAlloc_Node_i * const prevFreeNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, prevFreeNodeIndex, HbMem_FibAlloc_Node_i);
		HbReport_Assert_Assume(prevFreeNode->children_i[isLarger].isFree_i);
		HbReport_Assert_Assume(prevFreeNode->children_i[isLarger].childOrNextFreeNodeIndex_i == nextFreeNodeIndex);
		prevFreeNode->children_i[isLarger].childOrNextFreeNodeIndex_i = nodeIndex;
		nextFreeNode->prevFreeOrRecycledNodeIndex_i = nodeIndex;
		node->children_i[isLarger].childOrNextFreeNodeIndex_i = nextFreeNodeIndex;
		node->prevFreeOrRecycledNodeIndex_i = prevFreeNodeIndex;
	} else {
		node->children_i[isLarger].childOrNextFreeNodeIndex_i = node->prevFreeOrRecycledNodeIndex_i = nodeIndex;
		freeList->freeNodeIndices_i[isLarger] = nodeIndex;
	}
}

static void HbMem_FibAlloc_UnlinkNodeChildFromFreeList_i(HbMem_FibAlloc * const fibAlloc, size_t const nodeIndex, HbBool const isLarger, size_t const childLevel) {
	HbReport_Assert_Assume(fibAlloc != NULL);
	HbReport_Assert_Assume(childLevel <= fibAlloc->largestLevel_r);
	HbMem_FibAlloc_Node_i * const node = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, nodeIndex, HbMem_FibAlloc_Node_i);
	HbReport_Assert_Assume(node->children_i[isLarger].isFree_i);
	HbMem_FibAlloc_FreeList_i * const freeList = &fibAlloc->freeLists_i[childLevel];
	size_t const nextFreeNodeIndex = node->children_i[isLarger].childOrNextFreeNodeIndex_i;
	size_t const prevFreeNodeIndex = node->prevFreeOrRecycledNodeIndex_i;
	if (nextFreeNodeIndex != nodeIndex) {
		HbReport_Assert_Assume(prevFreeNodeIndex != nodeIndex);
		HbMem_FibAlloc_Node_i * const prevFreeNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, prevFreeNodeIndex, HbMem_FibAlloc_Node_i);
		HbReport_Assert_Assume(prevFreeNode->children_i[isLarger].isFree_i);
		HbReport_Assert_Assume(prevFreeNode->children_i[isLarger].childOrNextFreeNodeIndex_i == nodeIndex);
		prevFreeNode->children_i[isLarger].childOrNextFreeNodeIndex_i = nextFreeNodeIndex;
		HbMem_FibAlloc_Node_i * const nextFreeNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, nextFreeNodeIndex, HbMem_FibAlloc_Node_i);
		HbReport_Assert_Assume(nextFreeNode->children_i[isLarger].isFree_i);
		HbReport_Assert_Assume(nextFreeNode->prevFreeOrRecycledNodeIndex_i == nodeIndex);
		nextFreeNode->prevFreeOrRecycledNodeIndex_i = prevFreeNodeIndex;
		if (freeList->freeNodeIndices_i[isLarger] == nodeIndex) {
			freeList->freeNodeIndices_i[isLarger] = nextFreeNodeIndex;
		}
	} else {
		HbReport_Assert_Assume(prevFreeNodeIndex == nodeIndex);
		HbReport_Assert_Assume(freeList->freeNodeIndices_i[isLarger] == nodeIndex);
		freeList->freeNodeIndices_i[isLarger] = SIZE_MAX;
	}
}

void HbMem_FibAlloc_InitExplicit(HbMem_FibAlloc * const fibAlloc, size_t const largestLevel, HbMem_Tag * const tag,
                                 char const * const originNameImmutable, unsigned const originLocation) {
	HbReport_Assert_Assume(fibAlloc != NULL);
	HbReport_Assert_Assume(largestLevel < HbCountOf(HbMem_FibAlloc_Sizes));

	fibAlloc->largestLevel_r = largestLevel;

	HbMem_DynArray_InitExplicit(&fibAlloc->nodes_i, sizeof(HbMem_FibAlloc_Node_i), tag, originNameImmutable, originLocation);
	fibAlloc->lastRecycledNodeIndex_i = SIZE_MAX;

	fibAlloc->freeLists_i = (HbMem_FibAlloc_FreeList_i *) HbMem_Tag_AllocElementsExplicit(
			tag, sizeof(HbMem_FibAlloc_FreeList_i), largestLevel + 1, HbTrue, originNameImmutable, originLocation);
	for (size_t level = 0; level <= largestLevel; ++level) {
		HbMem_FibAlloc_FreeList_i * const freeList = &fibAlloc->freeLists_i[level];
		freeList->freeNodeIndices_i[0] = freeList->freeNodeIndices_i[1] = SIZE_MAX;
	}

	// Pre-allocate some memory for the first few allocations.
	HbMem_DynArray_ReserveForGrowing(&fibAlloc->nodes_i, largestLevel + 1);

	// Initialize the top-level node - which contains the entire tree as the larger node - to generalize the allocation logic.
	size_t const rootNodeIndex = HbMem_DynArray_Append(&fibAlloc->nodes_i, 1);
	HbReport_Assert_Assume(rootNodeIndex == 0);
	HbMem_FibAlloc_Node_i * const rootNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, rootNodeIndex, HbMem_FibAlloc_Node_i);
	rootNode->offset_i = 0;
	// Never use the smaller child, it doesn't exist.
	rootNode->children_i[0].isFree_i = HbFalse;
	rootNode->children_i[0].childOrNextFreeNodeIndex_i = HbMem_FibAlloc_Node_Child_ChildNodeIndex_Data_i;
	rootNode->children_i[1].isFree_i = HbTrue;
	// Only one entry, link to self.
	rootNode->children_i[1].childOrNextFreeNodeIndex_i = rootNodeIndex;
	rootNode->prevFreeOrRecycledNodeIndex_i = rootNodeIndex;
	fibAlloc->freeLists_i[largestLevel].freeNodeIndices_i[1] = rootNodeIndex;
}

void HbMem_FibAlloc_Shutdown(HbMem_FibAlloc * const fibAlloc) {
	HbReport_Assert_Assume(fibAlloc != NULL);
	HbMem_Tag_Free(fibAlloc->freeLists_i);
	HbMem_DynArray_Shutdown(&fibAlloc->nodes_i);
}

size_t HbMem_FibAlloc_Alloc(HbMem_FibAlloc * const fibAlloc, size_t const minimumCount, size_t const preferredCount, size_t * const allocationLevelOut) {
	HbReport_Assert_Assume(fibAlloc != NULL);
	HbReport_Assert_Assume(minimumCount != 0);
	HbReport_Assert_Assume((minimumCount == preferredCount || allocationLevelOut != NULL) && "If allocating a flexible amount, must handle the actual amount.");
	size_t const minimumLevel = HbSort_Find_FirstNotLess_Size(minimumCount, HbMem_FibAlloc_Sizes, HbCountOf(HbMem_FibAlloc_Sizes));
	if (minimumLevel > fibAlloc->largestLevel_r) {
		return HbMem_FibAlloc_Alloc_Failed;
	}
	size_t const preferredLevel = HbMath_Clamp_Size(HbSort_Find_FirstNotLess_Size(preferredCount, HbMem_FibAlloc_Sizes, HbCountOf(HbMem_FibAlloc_Sizes)),
	                                                minimumLevel, fibAlloc->largestLevel_r);

	size_t allocationLevel = preferredLevel;
	size_t freeLevel = SIZE_MAX;
	// Try to allocate on the preferred level - take a node on it or find a larger node (up to the head node with the whole tree as the larger node) to split.
	for (size_t level = preferredLevel; level <= fibAlloc->largestLevel_r; ++level) {
		HbMem_FibAlloc_FreeList_i const * const freeList = &fibAlloc->freeLists_i[level];
		if (freeList->freeNodeIndices_i[0] != SIZE_MAX || freeList->freeNodeIndices_i[1] != SIZE_MAX) {
			freeLevel = level;
			break;
		}
	}
	if (freeLevel == SIZE_MAX) {
		// Try to allocate on the largest available level not smaller than the minimum.
		while (allocationLevel > minimumLevel) {
			--allocationLevel;
			HbMem_FibAlloc_FreeList_i const * const freeList = &fibAlloc->freeLists_i[allocationLevel];
			if (freeList->freeNodeIndices_i[0] != SIZE_MAX || freeList->freeNodeIndices_i[1] != SIZE_MAX) {
				freeLevel = allocationLevel;
				break;
			}
		}
		if (freeLevel == SIZE_MAX) {
			return HbMem_FibAlloc_Alloc_Failed;
		}
	}

	// Take a free list entry on the closest level with one.
	HbMem_FibAlloc_FreeList_i * const largestFreeLevelFreeList = &fibAlloc->freeLists_i[freeLevel];
	// Prefer splitting smaller nodes to save larger nodes for bigger allocations, and also prefer older free nodes.
	HbBool freeChildIsLarger = largestFreeLevelFreeList->freeNodeIndices_i[0] == SIZE_MAX;
	size_t freeChildNodeIndex = largestFreeLevelFreeList->freeNodeIndices_i[freeChildIsLarger];
	HbMem_FibAlloc_UnlinkNodeChildFromFreeList_i(fibAlloc, freeChildNodeIndex, freeChildIsLarger, freeLevel);

	// Create a path from the closest level with a free node to the allocation level.
	while (freeLevel > allocationLevel) {
		// Create the new split node.
		HbBool const newNodeFromRecycled = fibAlloc->lastRecycledNodeIndex_i != SIZE_MAX;
		size_t newNodeIndex;
		if (newNodeFromRecycled) {
			newNodeIndex = fibAlloc->lastRecycledNodeIndex_i;
		} else {
			#ifdef HbMem_SizeMaxChecksNeeded
			// HbMem_DynArray_Append itself will check normally.
			if (SIZE_MAX / sizeof(HbMem_FibAlloc_Node_i) > HbMem_FibAlloc_MaxNodes_i && fibAlloc->nodes_i.count_r >= HbMem_FibAlloc_MaxNodes_i) {
				HbReport_Crash("Too many Fibonacci number block allocator nodes created, max 0x%zX.", HbMem_FibAlloc_MaxNodes_i);
			}
			#endif
			newNodeIndex = HbMem_DynArray_Append(&fibAlloc->nodes_i, 1);
		}
		HbMem_FibAlloc_Node_i * const newNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, newNodeIndex, HbMem_FibAlloc_Node_i);
		if (newNodeFromRecycled) {
			fibAlloc->lastRecycledNodeIndex_i = newNode->prevFreeOrRecycledNodeIndex_i;
		}
		HbMem_FibAlloc_Node_i * const freeChildNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, freeChildNodeIndex, HbMem_FibAlloc_Node_i);
		freeChildNode->children_i[freeChildIsLarger].isFree_i = HbFalse;
		freeChildNode->children_i[freeChildIsLarger].childOrNextFreeNodeIndex_i = newNodeIndex;
		newNode->offset_i = freeChildNode->offset_i + HbMem_FibAlloc_GetChildRelativeOffset_i(freeLevel, fibAlloc->largestLevel_r, freeChildIsLarger);
		// Add one child to the free list and take another one for the next iteration.
		HbBool const continueInLargerChild = freeLevel - allocationLevel <= 1; // Prefer splitting smaller nodes, only split the larger node on the smallest level if have to.
		HbMem_FibAlloc_AddNodeChildToFreeList_i(fibAlloc, newNodeIndex, !continueInLargerChild, HbMem_FibAlloc_GetChildLevel_i(freeLevel, !continueInLargerChild));
		freeChildIsLarger = continueInLargerChild;
		freeChildNodeIndex = newNodeIndex;
		freeLevel = HbMem_FibAlloc_GetChildLevel_i(freeLevel, continueInLargerChild);
	}

	HbMem_FibAlloc_Node_i * const allocationNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, freeChildNodeIndex, HbMem_FibAlloc_Node_i);
	allocationNode->children_i[freeChildIsLarger].isFree_i = HbFalse;
	allocationNode->children_i[freeChildIsLarger].childOrNextFreeNodeIndex_i = HbMem_FibAlloc_Node_Child_ChildNodeIndex_Data_i;
	if (allocationLevelOut != NULL) {
		*allocationLevelOut = allocationLevel;
	}
	return allocationNode->offset_i + HbMem_FibAlloc_GetChildRelativeOffset_i(allocationLevel, fibAlloc->largestLevel_r, freeChildIsLarger);
}

typedef struct HbMem_FibAlloc_PathStep_i {
	size_t isLarger_i : 1; // Whether this step corresponds to largerChild (of a narrower level) or smallerChild (of a broader level).
	size_t nodeIndex_i : (sizeof(size_t) * CHAR_BIT - 1);
	size_t childLevel_i; // Need to store explicitly because level 0 is the smaller child of both levels 1 and 2.
} HbMem_FibAlloc_PathStep_i;

// Starts from the child on fibAlloc->largestLevel_r. Returns the number of references, or 0 if failed (still overwrites in case of failure).
// pathOut must be large enough to store (fibAlloc->largestLevel_r + 1) elements, HbCountOf(HbMem_FibAlloc_Sizes) at most.
static size_t HbMem_FibAlloc_GetPathToAllocation_i(HbMem_FibAlloc const * const fibAlloc, size_t const allocation, HbMem_FibAlloc_PathStep_i pathOut[]) {
	HbReport_Assert_Assume(fibAlloc != NULL);
	HbReport_Assert_Assume(pathOut != NULL);
	size_t pathLength = 0;
	size_t nodeIndex = 0; // Start from the root, which contains the largest level as the larger child.
	size_t nodeLevel = fibAlloc->largestLevel_r + 1;
	for (;;) {
		HbMem_FibAlloc_Node_i const * const node = HbMem_DynArray_Get(&fibAlloc->nodes_i, nodeIndex, HbMem_FibAlloc_Node_i);
		size_t const largerChildOffset = node->offset_i + HbMem_FibAlloc_GetChildRelativeOffset_i(nodeLevel - 1, fibAlloc->largestLevel_r, HbTrue);
		HbBool continueInLargerChild = allocation >= largerChildOffset; // < - smaller, >= larger, >= 0 for the root where the only node is larger.
		size_t const childLevel = HbMem_FibAlloc_GetChildLevel_i(nodeLevel, continueInLargerChild);
		HbMem_FibAlloc_PathStep_i * const pathStep = &pathOut[pathLength];
		pathStep->isLarger_i = continueInLargerChild;
		pathStep->nodeIndex_i = nodeIndex;
		pathStep->childLevel_i = childLevel;
		++pathLength;
		HbMem_FibAlloc_Node_Child_i const * const child = &node->children_i[continueInLargerChild];
		if (child->isFree_i) {
			return 0;
		}
		if (child->childOrNextFreeNodeIndex_i == HbMem_FibAlloc_Node_Child_ChildNodeIndex_Data_i) {
			if (allocation != (continueInLargerChild ? largerChildOffset : node->offset_i)) {
				return 0; // Reached an allocation, but not what was asked.
			}
			break;
		}
		HbReport_Assert_Assume(nodeLevel != 0);
		nodeIndex = child->childOrNextFreeNodeIndex_i;
		nodeLevel = childLevel;
	}
	return pathLength;
}

void HbMem_FibAlloc_Free(HbMem_FibAlloc * const fibAlloc, size_t const allocation) {
	HbReport_Assert_Assume(fibAlloc != NULL);
	HbMem_FibAlloc_PathStep_i path[HbCountOf(HbMem_FibAlloc_Sizes)];
	size_t const pathLength = HbMem_FibAlloc_GetPathToAllocation_i(fibAlloc, allocation, path);
	if (pathLength == 0) {
		HbReport_Crash("Tried to free an allocation that wasn't created with HbMem_FibAlloc_Alloc or has already been freed (%zu).", allocation);
	}
	// Combine split nodes into free nodes while both children are now free.
	size_t pathRemaining = pathLength;
	while (pathRemaining > 0) {
		size_t const pathStepIndex = --pathRemaining;
		HbMem_FibAlloc_PathStep_i const * const pathStep = &path[pathStepIndex];
		HbMem_FibAlloc_Node_i * const pathStepNode = HbMem_DynArray_GetMut(&fibAlloc->nodes_i, pathStep->nodeIndex_i, HbMem_FibAlloc_Node_i);
		if (!pathStepNode->children_i[!pathStep->isLarger_i].isFree_i) {
			HbMem_FibAlloc_AddNodeChildToFreeList_i(fibAlloc, pathStep->nodeIndex_i, (HbBool) pathStep->isLarger_i, pathStep->childLevel_i);
			break;
		}
		// If both are now free, destroy (recycle) the child node - first removing the sibling from the free list.
		HbReport_Assert_Assume(pathStepIndex != 0); // On the largest level, the sibling is never free.
		HbMem_FibAlloc_UnlinkNodeChildFromFreeList_i(fibAlloc, pathStep->nodeIndex_i, !pathStep->isLarger_i,
		                                             HbMem_FibAlloc_GetChildLevel_i(path[pathStepIndex - 1].childLevel_i, !pathStep->isLarger_i));
		// Recycle the node with both now free children.
		pathStepNode->prevFreeOrRecycledNodeIndex_i = fibAlloc->lastRecycledNodeIndex_i;
		fibAlloc->lastRecycledNodeIndex_i = pathStep->nodeIndex_i;
	}
}
