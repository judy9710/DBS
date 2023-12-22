#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "minirel.h"
#include "bf.h"

typedef struct BFpage{
  PFpage         fpage;       /* page data from the file                 */
  struct BFpage  *nextpage;   /* next in the linked list of buffer pages */
  struct BFpage  *prevpage;   /* prev in the linked list of buffer pages */
  bool_t         dirty;       /* TRUE if page is dirty                   */
  short          count;       /* pin count associated with the page      */
  int            pageNum;     /* page number of this page                */
  int            fd;          /* PF file descriptor of this page         */
  int            unixfd;      /* Unix file descriptor of this page       */
} BFpage; /* LRU와 Free list로 사용될 수 있는 구조체 정의 */

typedef struct BFhash_entry {
  struct BFhash_entry *nextentry;     /* next hash table element or NULL */
  struct BFhash_entry *preventry;     /* prev hash table element or NULL */
  int fd;                             /* file descriptor                 */
  int pageNum;                        /* page number                     */
  struct BFpage *bpage;               /* ptr to buffer holding this page */
} BFhash_entry; /* Hash table로 사용될 수 있는 구조체 정의 */


// void BF_Init(void){
//     BFpage* freelist = NULL; /* 첫 노드를 가리킬 헤드 포인터를 선언하고 NULL을 초기값으로 대입 */
//     for (int i=0; i<BF_MAX_BUFS; i++){
//       BFpage* curr = freelist; /* head 가 움직일 수 없으니까 curr 변수로 list traversal */
//       BFpage* prev = NULL;
//       while (curr != NULL) {
//         prev=curr;
//         curr = curr->nextpage;
//       }
//       /* currnext 엔트리의 초기화 */
//       BFpage* currnext = (BFpage*)malloc(sizeof(BFpage));
//       currnext->prevpage = curr;
//       currnext->dirty=FALSE;
//       currnext->count=0;
//       currnext->pageNum=-1;
//       currnext->fd=-1;
//       currnext->unixfd=-1;
    
//       if(i==0){
//         freelist=currnext;
//       }
//       else{
//         prev->nextpage=currnext;

//       }
      
      
//     }
    
//     struct BFhash_entry hashtablehead; /* initializes the hash table */
// }

void BF_Init(void){
    BFpage* freelist = NULL; /* 첫 노드를 가리킬 헤드 포인터를 선언하고 NULL 을 초기값으로 대입 */
    for (int i=0; i<BF_MAX_BUFS; i++){
        BFpage* curr = (BFpage*)malloc(sizeof(BFpage));
        curr->dirty=FALSE;
        curr->count=0;
        curr->pageNum=-1;
        curr->fd=-1;
        curr->unixfd=-1;
        /* 연결 리스트가 비어있는 경우 */
        if(i==0){
          freelist=curr;
        }
        /* 연결 리스트가 비어있지 않은 경우 */
        else{
          curr->nextpage=freelist;
          freelist=curr; /* head포인터가 새로운 노드를 가리키도록 */
          curr->nextpage->prevpage=curr;
        }
      
    }
}