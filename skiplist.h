#ifndef __SKIPLIST__
#define __SKIPLIST__

#include <stdint.h>

#define SKIPLIST_OK 1
#define SKIPLIST_ERROR 0

typedef int32_t SlKey_t;

struct SkipListNode_t {
  SlKey_t slKey;
  void *ptrValue;
  struct SkipListNode_t **rgPtrSlnForward;
  int32_t *rgCLinkWidth;
  int16_t cLevel;
};

typedef struct SkipListNode_t SkipListNode_t;

typedef struct {
  int32_t cMaxLevel;
  int32_t cBits;
  SkipListNode_t* ptrSlnHeader;
  int32_t cSize;
} SkipList_t;

SkipList_t* skiplist_alloc(int32_t cMaxLevel, int32_t cBits);
void skiplist_free(SkipList_t *ptrSl);

int32_t skiplist_size(SkipList_t *ptrSl);
void skiplist_insert(SkipList_t *ptrSl, SlKey_t slKey, void *ptrValue);
int32_t skiplist_delete(SkipList_t *ptrSl, SlKey_t slKey);
void* skiplist_find(SkipList_t *ptrSl, SlKey_t slKey);
int32_t skiplist_index(SkipList_t *ptrSl, int32_t ix,
		       SlKey_t *ptrSlKeyOut, void **ptrPtrValueOut);
void skiplist_debug_levels(SkipList_t *ptrSl, int32_t* rgC, int32_t cSize);
void skiplist_debug_print(SkipList_t *ptrSl);

#endif // ifndef __SKIPLIST__
