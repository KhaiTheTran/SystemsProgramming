/*
 * Copyright ©2018 Justin Hsia.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2018 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "memindex.h"
#include "libhw1/CSE333.h"
#include "libhw1/HashTable.h"

// This helper function is passed to SortLinkedList();
// it is the comparator that compares the rank of two
// search results while sorting the result list.
static int MISearchListComparator(LLPayload_t e1, LLPayload_t e2) {
  SearchResult *sr1 = (SearchResult *) e1;
  SearchResult *sr2 = (SearchResult *) e2;

  if (sr1->rank > sr2->rank)
    return 1;
  if (sr1->rank < sr2->rank)
    return -1;
  return 0;
}

// Used by MIListFree() to free a linked list of positions.
static void MINullFree(LLPayload_t ptr) { }

// Frees a linked list of positions.
static void MIListFree(HTValue_t ptr) {
  LinkedList list = (LinkedList) ptr;
  FreeLinkedList(list, &MINullFree);
}

// Frees a WordDocSet structure.
static void MIFree(HTValue_t ptr) {
  WordDocSetPtr wd = (WordDocSetPtr)ptr;
  free(wd->word);
  FreeHashTable(wd->docIDs, &MIListFree);
  free(wd);
}

MemIndex AllocateMemIndex(void) {
  // Happily, HashTables dynamically resize themselves
  // now, so we can start by allocating a small hashtable.
  HashTable mi = AllocateHashTable(128);
  Verify333(mi != NULL);
  return mi;
}

void FreeMemIndex(MemIndex index) {
  FreeHashTable(index, &MIFree);
}

HWSize_t MINumWordsInMemIndex(MemIndex index) {
  return NumElementsInHashTable(index);
}

int MIAddPostingList(MemIndex index, char *word, DocID_t docid,
                     LinkedList positions) {
  HTKey_t wordkey = FNVHash64((unsigned char *) word, strlen(word));
  HTKeyValue kv, hitkv;
  WordDocSet *wds;
  int res;

  // STEP 1.
  // Remove this "return 1;". We added this in here
  // so that your filecrawler unit tests would pass
  // even though you hadn't yet finished the
  // memindex.c implementation.

  // return 1;

  // First, we have to see if the word we're being handed
  // already exists in the inverted index.
  res = LookupHashTable(index, wordkey, &kv);
  if (res == 0) {
    // STEP 2.
    // No, this is the first time the inverted index has
    // seen this word.  We need to malloc and prepare a
    // new WordDocSet structure.  After malloc'ing it,
    // we need to:
    //   (1) insert the word into the WordDocSet,
    //   (2) allocate a new hashtable for the docID->positions mapping,
    //   (3) insert that hashtable into the WordDocSet, and
    //   (4) insert the the new WordDocSet into the inverted
    //       index (i.e., into the "index" table).
    wds = (WordDocSet*) malloc(sizeof(WordDocSet));
    Verify333(wds != NULL);
    // insert the word into the WordDocSet
    wds->word = word;

    // allocate a new hashtable for the docID->positions
    wds->docIDs = AllocateHashTable(128);
    Verify333(wds->docIDs != NULL);

    // insert that hashtable into the WordDocSet
    kv.key = wordkey;
    kv.value = (void*)wds;
    // insert the the new WordDocSet into the inverted
    res = InsertHashTable(index, kv, &hitkv);
    Verify333(res == 1);
    } else {
    // Yes, this word already exists in the inverted index.
    // So, there's no need to insert it again; we can go
    // ahead and free the word.
    free(word);

    // Instead of allocating a new WordDocSet, we'll
    // use the one that's already in the inverted index.
    wds = (WordDocSet *) kv.value;
  }

  // Verify that the docID doesn't exist in the WordDocSet.
  res = LookupHashTable(wds->docIDs, docid, &hitkv);
  Verify333(res == 0);

  // STEP 3.
  // Insert a new entry into the wds->docIDs hash table.
  // The entry's key is this docID and the entry's value
  // is the "positions" word positions list we were passed
  // as an argument.

  // The entry's key is this docID and the entry's value
  kv.key = docid;
  // is the "positions" word positions list
  kv.value = (void*) positions;
  // Insert a new entry into the wds->docIDs hash table
  res = InsertHashTable(wds->docIDs, kv, &hitkv);
  Verify333(res == 1);
  return 1;
}

LinkedList MIProcessQuery(MemIndex index, char *query[], uint8_t qlen) {
  LinkedList retlist;
  HTKeyValue kv;
  WordDocSet *wds;
  int res;
  HTKey_t wordkey;

  // If the user provided us with an empty search query, return NULL
  // to indicate failure.
  if (qlen == 0)
    return NULL;

  // Allocate a linked list to store our search results.  A search
  // result is just a list of SearchResult structures.
  retlist = AllocateLinkedList();
  Verify333(retlist != NULL);

  // STEP 4.
  // The most interesting part of Part C starts here...!
  //
  // Look up the first query word (query[0]) in the inverted
  // index.  For each document that matches, allocate a SearchResult
  // structure.  Initialize that SearchResult structure with the
  // docID, and the initial computed rank for the document.  (The
  // initial computed rank is the number of times the word appears
  // in that document.)
  //
  // Then, append the SearchResult structure onto retlist.
  //
  // If there are no matching documents, free retlist and return NULL.

  // Look up the first query word (query[0]) in the inverted
  wordkey =  FNVHash64((unsigned char *) query[0], strlen(query[0]));
  res = LookupHashTable(index, wordkey, &kv);
  Verify333(res != -1);
  // if not, free the list
  if (res == 0) {
    FreeLinkedList(retlist, &free);
    return NULL;
  }
  // create iterator to document
  wds = (WordDocSet*) kv.value;
  HTIter hiter = HashTableMakeIterator(wds->docIDs);
  Verify333(hiter != NULL);

  while (!HTIteratorPastEnd(hiter)) {
    // set document to kv
    res = HTIteratorGet(hiter, &kv);
    Verify333(res == 1);

    //  Initialize that SearchResult structure
    SearchResult *search = (SearchResult*) malloc(sizeof(SearchResult));
    Verify333(search != NULL);
    // Initialize docID, and the initial computed rank for the document
    search->docid = kv.key;
    search->rank = (uint32_t) NumElementsInLinkedList((LinkedList) kv.value);

    // append the SearchResult structure onto retlist
    Verify333(AppendLinkedList(retlist, search));
    HTIteratorNext(hiter);
  }
  HTIteratorFree(hiter);
  // Great; we have our search results for the first query
  // word.  If there is only one query word, we're done!
  // Sort the result list and return it to the caller.
  if (qlen == 1) {
    SortLinkedList(retlist, 0, &MISearchListComparator);
    return retlist;
  }

  // OK, there are additional query words.  Handle them one
  // at a time.
  int i;
  for (i = 1; i < qlen; i++) {
    LLIter llit;
    int j, ne;

    // STEP 5.
    // Look up the next query word (query[i]) in the inverted index.
    // If there are no matches, it means the overall query
    // should return no documents, so free retlist and return NULL.

    // Look up the next query word (query[i]) in the inverted index
    wordkey =  FNVHash64((unsigned char *) query[i], strlen(query[i]));
    res = LookupHashTable(index, wordkey, &kv);
    Verify333(res != -1);
    // if not found free the list
    if (res == 0) {
      FreeLinkedList(retlist, &free);
      return NULL;
    }
    // STEP 6.
    // There are matches.  We're going to iterate through
    // the docIDs in our current search result list, testing each
    // to see whether it is also in the set of matches for
    // the query[i].
    //
    // If it is, we leave it in the search
    // result list and we update its rank by adding in the
    // number of matches for the current word.
    //
    // If it isn't, we delete that docID from the search result list.
    wds = (WordDocSet*) kv.value;
    llit = LLMakeIterator(retlist, 0);
    Verify333(llit != NULL);
    ne = NumElementsInLinkedList(retlist);
    for (j = 0; j < ne; j++) {
      // going to iterate through the docIDs in our current search result list
      SearchResult *search;
      // get payload from list
      LLIteratorGetPayload(llit, (void**) &search);

      // check the currect document in the retlist,
      // in the worddocset in the query[i]
      res = LookupHashTable(wds->docIDs, search->docid, &kv);
      Verify333(res != -1);
      if (res == 0) {
        // eleminate document without contain the query[i]
        res = LLIteratorDelete(llit, &free);
      } else {
        // the search result list and we update its rank by adding
        search->rank += (uint32_t)
        NumElementsInLinkedList((LinkedList) kv.value);
        LLIteratorNext(llit);
      }
    }
    LLIteratorFree(llit);

    // We've finished processing this query word.
    // If there are no documents left in our query result list,
    // free retlist and return NULL.
    if (NumElementsInLinkedList(retlist) == 0) {
      FreeLinkedList(retlist, (LLPayloadFreeFnPtr)free);
      return NULL;
    }
  }

  // Sort the result list by rank and return it to the caller.
  SortLinkedList(retlist, 0, &MISearchListComparator);
  return retlist;
}
