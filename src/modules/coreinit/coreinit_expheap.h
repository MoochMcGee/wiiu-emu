#pragma once
#include "systemtypes.h"
#include "coreinit_memory.h"

struct ExpandedHeap;

ExpandedHeap *
MEMCreateExpHeap(ExpandedHeap *heap, uint32_t size);

ExpandedHeap *
MEMCreateExpHeapEx(ExpandedHeap *heap, uint32_t size, uint16_t flags);

ExpandedHeap *
MEMDestroyExpHeap(ExpandedHeap *heap);

void *
MEMAllocFromExpHeap(ExpandedHeap *heap, uint32_t size);

void *
MEMAllocFromExpHeapEx(ExpandedHeap *heap, uint32_t size, int alignment);

void
MEMFreeToExpHeap(ExpandedHeap *heap, void *block);

HeapMode
MEMSetAllocModeForExpHeap(ExpandedHeap *heap, HeapMode mode);

HeapMode
MEMGetAllocModeForExpHeap(ExpandedHeap *heap);

uint32_t
MEMAdjustExpHeap(ExpandedHeap *heap);

uint32_t
MEMResizeForMBlockExpHeap(ExpandedHeap *heap, p32<void> address, uint32_t size);

uint32_t
MEMGetTotalFreeSizeForExpHeap(ExpandedHeap *heap);

uint32_t
MEMGetAllocatableSizeForExpHeap(ExpandedHeap *heap);

uint32_t
MEMGetAllocatableSizeForExpHeapEx(ExpandedHeap *heap, int alignment);

uint16_t
MEMSetGroupIDForExpHeap(ExpandedHeap *heap, uint16_t id);

uint16_t
MEMGetGroupIDForExpHeap(ExpandedHeap *heap);

uint32_t
MEMGetSizeForMBlockExpHeap(p32<void> addr);

uint16_t
MEMGetGroupIDForMBlockExpHeap(p32<void> addr);

HeapDirection
MEMGetAllocDirForMBlockExpHeap(p32<void> addr);