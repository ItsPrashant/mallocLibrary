#ifndef _MALLOC_H
#define MALLOC_H

#define MAX_MEM 1048576
#define LEVEL 15
#define MEM_OFFSET 8
typedef unsigned long long ull;
#define LARGE_CHUNKS 20
#define MIN_LARGE_CHUNK 257

struct mallinfo{
int arena;
ull totalMemReq;
ull totalFreeMemReq;
ull usedBlocks;
ull freeBlocks;
ull totBlocks;
} arenaStats;

struct mynode
{
	int level;
	int isFree;
	struct mynode *nxt;
	struct mynode *prev;
	
};

struct mallocArena
{
	void *startOfHeap,*endOfHeap;
	struct mallocArena *nxt,*prev;
};
typedef struct mallocArena mallocArena;
typedef struct mynode node;

__thread mallocArena *arenaHead=NULL,*arenaTail=NULL;
__thread pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread node *largeChunksListHead[LARGE_CHUNKS],*largeChunksListTail[LARGE_CHUNKS];

struct mynode *head[LEVEL],*tail[LEVEL];

extern void insertNewHeap(mallocArena *ptr);
extern void requestMoreMem();
extern int getNoOfCores();
extern void initBuddy();
extern void traverseList(int level);
extern void displayWholeList();
extern void insertInListHead(node *ptr,int level);
extern int findLevelForAlloc(int mem);
extern void *findStartOfHeap(node *ptr);
extern node *findFreeBlock(int level);
extern void insertInListTail(node *ptr,int level);
extern node *findAndBreakBlock(int level);
extern int getApproxPages(size_t memSize);
extern void addMemPageInLargeChunk(node *ptr);
extern node *removeLargeChunk(size_t memSize);

extern node *addLargeChunk(size_t memSize);
extern void *malloc(size_t memSize);

extern node *findBuddy(node* p,int level);
extern node *detachBlock(node *p,int level);
extern void coalesceBuddies(node *p);
extern void unmapLargeChunk(node *p);
extern void free(void *fptr);
extern void *realloc(void *ptr,size_t memSize);

extern void *calloc(size_t nMem,size_t size);

#endif
