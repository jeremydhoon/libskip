#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "skiplist.h"

void print_levels(SkipList_t *ptrSl) {
  int cSize = skiplist_size(ptrSl);
  int32_t *rgC = (int32_t*)malloc(sizeof(int32_t)*cSize);
  skiplist_debug_levels(ptrSl,rgC,cSize);
  int32_t i;
  printf("Skiplist Levels: ");
  for (i = 0; i < cSize; i++) {
    printf("%d ", rgC[i]);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  srand(clock());
  SkipList_t *ptrSl = skiplist_alloc(10,1);
  int64_t i;
  for (i = 1; i <= 10; i++) {
    skiplist_insert(ptrSl, i, (void*)(i*i));
    SlKey_t slKey;
    int64_t cValue;
    if (skiplist_index(ptrSl, (i+1)/2-1, &slKey, (void**)(&cValue))) {
      assert(slKey == (i+1)/2);
    } else {
      assert(0);
    }
  }
  for (i = 0; i < 10; i++) {
    SlKey_t slKey;
    int64_t cValue;
    if (skiplist_index(ptrSl, i, &slKey, (void**)(&cValue))) {
      assert(slKey == i+1);
    } else {
      assert(0);
    }
  }
  int32_t ixRm = 5;
  assert(skiplist_delete(ptrSl, ixRm));
  for (i = 1; i <= 10; i++) {
    int64_t cValue = (int64_t)skiplist_find(ptrSl, i);
    if (i != ixRm) {
      assert(cValue == i*i);
    } else {
      assert(cValue == 0);
    }
  }
  for (i = 1; i <= 10; i+=2) {
    int32_t fSuccess = skiplist_delete(ptrSl,i);
    if (i != ixRm) {
      assert(fSuccess);
    } else {
      assert(!fSuccess);
    }
  }
  skiplist_free(ptrSl);
  return 1;
}
