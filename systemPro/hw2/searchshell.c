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

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"

#define SIZE 1024
static void Usage(void);
static void searchIter(LinkedList searchresult, DocTable tabdoc);
int main(int argc, char **argv) {
  if (argc != 2)
    Usage();

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - crawl from a directory provided by argv[1] to produce and index
  //  - prompt the user for a query and read the query from stdin, in a loop
  //  - split a query into words (check out strtok_r)
  //  - process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.

  // declare varible
  MemIndex index;
  LinkedList searchresult;
  char query[SIZE];
  char *token;
  char *addptr;
  DocTable tabdoc;

  //  crawl from a directory provided by argv[1] to produce and index
  //  do not thing if not match
  printf("Indexing: \'%s\' \n", argv[1]);
  if (CrawlFileTree(argv[1], &tabdoc, &index)== 0) {
    Usage();
  }
  // check data after crawl
  Verify333(tabdoc != NULL);
  Verify333(index != NULL);

  // prompt the user for a query and read the query from stdin, in a loop
  while (1) {
    printf("enter query:\n");
    // exit when detects cntrl-D from the keyboard
    if (fgets(query, SIZE, stdin) != NULL) {
      uint32_t qlen = 0;
      char *inStr = query;
      // allocate memory for array for split a query into words
      char** queArr = (char**) malloc(sizeof(token)*(SIZE/2));
      Verify333(queArr != NULL);

      // split words by spaces
      while (1) {
        token = strtok_r(inStr, " ", &addptr);
        if (token == NULL) {
          // stop split when go to the end of input string
          break;
        }
        queArr[qlen] = token;
        qlen++;
        inStr = NULL;
      }
     // because  fgets() include "\n" in the input
     // so we replace by '\0'
     // navigate '\n'
     char *n = strchr(queArr[qlen-1], '\n');
     if (n != NULL) {
       *n = '\0';
     }
     searchresult = MIProcessQuery(index, queArr, qlen);
     if (searchresult != NULL)
       searchIter(searchresult, tabdoc);
    free(queArr);
    } else {
      break;
    }
  }
  FreeDocTable(tabdoc);
  FreeMemIndex(index);
  return EXIT_SUCCESS;
}

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}
static void searchIter(LinkedList searchresult, DocTable tabdoc) {
  LLIter iterL = LLMakeIterator(searchresult, 0);
  Verify333(iterL != NULL);
  // output match document from input
  bool ll = true;
  SearchResult *sr;
  while (ll) {
    LLIteratorGetPayload(iterL, (void**) &sr);
    printf("  %s (%u)\n", DTLookupDocID(tabdoc, sr->docid), sr->rank);
    ll = LLIteratorNext(iterL);
  }
  LLIteratorFree(iterL);
  FreeLinkedList(searchresult, &free);
}
