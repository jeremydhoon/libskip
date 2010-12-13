// skiplist.c -- implement skiplist functionality

#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "skiplist.h"

#define SKIPLIST_MAX_RANDOM_BITS 32

static int32_t randbits(int32_t cBits);
static SkipListNode_t* skiplist_alloc_node(int cMaxLevel);
static void skiplist_free_node(SkipListNode_t* ptrSln);
static int32_t generate_level(SkipList_t *ptrSl);
static SkipListNode_t** obtain_work_buffer(SkipList_t *ptrSl);
static void release_work_buffer(SkipListNode_t** rgPtrSln);

SkipList_t* skiplist_alloc(int32_t cMaxLevel, int cBits) {
  assert(cMaxLevel > 0);
  SkipList_t* ptrSl = (SkipList_t*)malloc(sizeof(SkipList_t));
  ptrSl->cMaxLevel = cMaxLevel;
  ptrSl->cBits = cBits;
  ptrSl->ptrSlnHeader = skiplist_alloc_node(ptrSl->cMaxLevel);
  ptrSl->ptrSlnHeader->cLevel = 0;
  return ptrSl;
}

void skiplist_free(SkipList_t *ptrSl) {
  SkipListNode_t *ptrSlnCurr, *ptrSlnNext;
  ptrSlnCurr = ptrSl->ptrSlnHeader->rgPtrSlnForward[0];
  ptrSlnNext = NULL;
  while (ptrSlnCurr) {
    ptrSlnNext = ptrSlnCurr->rgPtrSlnForward[0];
    skiplist_free_node(ptrSlnCurr);
    ptrSlnCurr = ptrSlnNext;
  }
  free(ptrSl);
}

int32_t skiplist_size(SkipList_t *ptrSl) {
  return ptrSl->cSize;
}

void skiplist_insert(SkipList_t *ptrSl, SlKey_t slKey, void *ptrValue) {
  int32_t cMaxLevel = ptrSl->cMaxLevel;
  int32_t ixLevel = generate_level(ptrSl);
  SkipListNode_t *ptrSlnNew = skiplist_alloc_node(cMaxLevel);
  ptrSlnNew->cLevel = (int16_t)ixLevel;
  ptrSlnNew->slKey = slKey;
  ptrSlnNew->ptrValue = ptrValue;

  SkipListNode_t **rgPtrSlnBuf = obtain_work_buffer(ptrSl);

  SkipListNode_t *ptrSlnCurr = ptrSl->ptrSlnHeader;
  int32_t i;
  for (i = ptrSl->cMaxLevel-1; i >= 0; i--) {
    SkipListNode_t *ptrSlnNext;
    int32_t cLinkWidth = 0;
    while ((ptrSlnNext = ptrSlnCurr->rgPtrSlnForward[i])
	   && ptrSlnNext->slKey < slKey) {
      cLinkWidth += ptrSlnCurr->rgCLinkWidth[i];
      ptrSlnCurr = ptrSlnNext;
    }
    rgPtrSlnBuf[i] = ptrSlnCurr;
    if (i < cMaxLevel - 1) {
      ptrSlnNew->rgCLinkWidth[i+1] = cLinkWidth;
    }
  }
  ptrSlnNew->rgCLinkWidth[0] = 1;
  int32_t cLinkWidth = 0;
  for (i = 0; i <= ixLevel; i++) {
    ptrSlnCurr = rgPtrSlnBuf[i];
    ptrSlnNew->rgPtrSlnForward[i] = ptrSlnCurr->rgPtrSlnForward[i];
    ptrSlnCurr->rgPtrSlnForward[i] = ptrSlnNew;

    int32_t cOldWidth = ptrSlnCurr->rgCLinkWidth[i];
    int32_t cTraveled = ptrSlnNew->rgCLinkWidth[i];
    cLinkWidth += cTraveled;
    int32_t cNewWidth = cOldWidth - cLinkWidth;
    ptrSlnCurr->rgCLinkWidth[i] = cLinkWidth;
    ptrSlnNew->rgCLinkWidth[i] = cNewWidth < 0 ? 0 : cNewWidth;
  }  
  for (i = ixLevel + 1; i < cMaxLevel; i++) {
    rgPtrSlnBuf[i]->rgCLinkWidth[i]++;
  }
  ptrSl->cSize++;
  release_work_buffer(rgPtrSlnBuf);
}

int32_t skiplist_delete(SkipList_t *ptrSl, SlKey_t slKey) {
  SkipListNode_t **rgPtrSlnBuf = obtain_work_buffer(ptrSl);
  SkipListNode_t *ptrSlnCurr,*ptrSlnNext;
  int32_t cMaxLevel = ptrSl->cMaxLevel;
  int32_t i;
  int32_t fFound = 0;
  ptrSlnCurr = ptrSl->ptrSlnHeader;
  for (i = cMaxLevel-1; i >= 0; i--) {
    while ((ptrSlnNext = ptrSlnCurr->rgPtrSlnForward[i])
	   && ptrSlnNext->slKey < slKey) {
      ptrSlnCurr = ptrSlnNext;
    }
    rgPtrSlnBuf[i] = ptrSlnCurr;
    fFound |= ptrSlnNext && ptrSlnNext->slKey == slKey;
  }  
  if (!fFound) {
    return SKIPLIST_ERROR;
  }
  SkipListNode_t *ptrSlnToDelete = rgPtrSlnBuf[0]->rgPtrSlnForward[0];
  for (i = 0; i <= ptrSlnToDelete->cLevel; i++) {
    ptrSlnCurr = rgPtrSlnBuf[i];
    ptrSlnCurr->rgPtrSlnForward[i] = ptrSlnToDelete->rgPtrSlnForward[i];
    ptrSlnCurr->rgCLinkWidth[i] = (ptrSlnCurr->rgCLinkWidth[i]
				   + ptrSlnToDelete->rgCLinkWidth[i] - 1);    
  }
  for (i = ptrSlnToDelete->cLevel+1; i < cMaxLevel; i++) {
    rgPtrSlnBuf[i]->rgCLinkWidth[i]--;
  }
  ptrSl->cSize--;
  return SKIPLIST_OK;
  release_work_buffer(rgPtrSlnBuf);
}

void* skiplist_find(SkipList_t* ptrSl, SlKey_t slKey) {
  const int32_t cMaxLevel = ptrSl->cMaxLevel;
  int32_t i;
  SkipListNode_t *ptrSlnCurr = ptrSl->ptrSlnHeader;
  for (i = cMaxLevel-1; i >= 0; i--) {
    SkipListNode_t *ptrSlnNext;
    while ((ptrSlnNext = ptrSlnCurr->rgPtrSlnForward[i])
	   && ptrSlnNext->slKey < slKey) {
      ptrSlnCurr = ptrSlnNext;
    }
    if (ptrSlnNext && ptrSlnNext->slKey == slKey) {
      return ptrSlnNext->ptrValue;
    }
  }
  return NULL;
}

int32_t skiplist_index(SkipList_t *ptrSl, int32_t ix, SlKey_t *ptrSlKeyOut,
		       void** ptrPtrValueOut) {
  const int32_t cMaxLevel = ptrSl->cMaxLevel;
  SkipListNode_t *ptrSlnCurr = ptrSl->ptrSlnHeader;
  int32_t cToTravel = ix+1;
  int32_t cTraveled = 0;
  int32_t i;
  for (i = cMaxLevel-1; i >= 0; i--) {
    SkipListNode_t *ptrSlnNext;
    int32_t cLinkWidth = ptrSlnCurr->rgCLinkWidth[i];
    while ((ptrSlnNext = ptrSlnCurr->rgPtrSlnForward[i])
	   && (cTraveled + cLinkWidth <= cToTravel)) {
      cTraveled += cLinkWidth;
      ptrSlnCurr = ptrSlnNext;
      cLinkWidth = ptrSlnCurr->rgCLinkWidth[i];
    }
    if (cTraveled == cToTravel) {
      *ptrSlKeyOut = ptrSlnCurr->slKey;
      *ptrPtrValueOut = ptrSlnCurr->ptrValue;
      return SKIPLIST_OK;
    }
  }
  return SKIPLIST_ERROR;
}

void skiplist_debug_levels(SkipList_t *ptrSl, int32_t *rgC, int32_t cSize) {
  assert(cSize == skiplist_size(ptrSl));
  int32_t i = 0;
  SkipListNode_t *ptrSln = ptrSl->ptrSlnHeader->rgPtrSlnForward[0];
  while (i < cSize && ptrSln) {
    rgC[i++] = ptrSln->cLevel;
    ptrSln = ptrSln->rgPtrSlnForward[0];
  }
}

void skiplist_debug_print(SkipList_t *ptrSl) {
  int32_t cMaxLevel = ptrSl->cMaxLevel;
  int32_t i;
  for (i = cMaxLevel-1; i >= 0; i--) {
    SkipListNode_t *ptrSln = ptrSl->ptrSlnHeader;
    while (ptrSln) {
      int32_t cLinkWidth = ptrSln->rgCLinkWidth[i];
      SlKey_t slKey = ptrSln->slKey;
      printf("%d(%d)", slKey, cLinkWidth);
      int j;
      for (j = 0; j < cLinkWidth; j++) {
	printf("\t");
      }
      ptrSln = ptrSln->rgPtrSlnForward[i];
    }
    printf("\n");
  }
}

static SkipListNode_t *skiplist_alloc_node(int32_t cMaxLevel) {
  SkipListNode_t* ptrSln = (SkipListNode_t*)malloc(sizeof(SkipListNode_t));

  int32_t cFwdBytes = cMaxLevel*(sizeof(SkipListNode_t*));
  SkipListNode_t** rgPtrSlnForward = (SkipListNode_t**)malloc(cFwdBytes);
  memset((void*)rgPtrSlnForward, 0, cFwdBytes);

  int32_t cWidthBytes = cMaxLevel*(sizeof(int32_t));
  int32_t *rgCLinkWidth = (int32_t*)malloc(cWidthBytes);
  memset((void*)rgCLinkWidth, 0, cWidthBytes);

  ptrSln->rgPtrSlnForward = rgPtrSlnForward;
  ptrSln->slKey = 0;
  ptrSln->ptrValue = NULL;
  ptrSln->rgCLinkWidth = rgCLinkWidth;
  return ptrSln;
}

static void skiplist_free_node(SkipListNode_t *ptrSln) {
  free(ptrSln->rgPtrSlnForward);
  free(ptrSln->rgCLinkWidth);
  free(ptrSln);
}

static int32_t cRandomBitsRemaining = 0;
static int32_t randbits(int32_t cBits) {
  static int32_t i;
  if (cBits > cRandomBitsRemaining) {
    cRandomBitsRemaining = SKIPLIST_MAX_RANDOM_BITS;
    i = rand();
  }
  int32_t cMask = ~(~0 << cBits);
  assert(cMask < (1 << cBits));
  int32_t cRet = i & cMask;
  i >>= cBits;
  cRandomBitsRemaining -= cBits;
  return cRet;
}

static int32_t generate_level(SkipList_t *ptrSl) {
  int cLevel = 0;
  while (randbits(ptrSl->cBits) && cLevel < ptrSl->cMaxLevel-1) {
    cLevel++;
  }
  assert(cLevel < ptrSl->cMaxLevel);
  return cLevel;
}

static SkipListNode_t** obtain_work_buffer(SkipList_t *ptrSl) {
  int cBytes = ptrSl->cMaxLevel*(sizeof(SkipListNode_t*));
  return (SkipListNode_t**)malloc(cBytes);
}
static void release_work_buffer(SkipListNode_t** rgPtrSln) {
  free(rgPtrSln);
}
