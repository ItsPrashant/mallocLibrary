#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
//#include <stdlib.h>
#include "malloc.h"
#include <string.h>


int getNoOfCores()
{
	int totalCores=sysconf(_SC_NPROCESSORS_ONLN);
	return totalCores;
}

void initBuddy()
{
	for(int i=0;i<LEVEL;i++){
		head[i]=NULL;
		tail[i]=NULL;
	}
	requestMoreMem();
	arenaStats.totBlocks++;
	arenaStats.freeBlocks++;
	for(int i=0;i<LARGE_CHUNKS;i++)
	{
		largeChunksListHead[i]=NULL;
		largeChunksListTail[i]=NULL;
		
	}

}


void traverseList(int level)
{
	node *ptr=head[level];
	while(ptr!=NULL)
	{
		//printf(" (L:%d,F:%d,N:%p) ",ptr->level,ptr->isFree,ptr->nxt);
		ptr=ptr->nxt;
	}
	//printf("\n");
}

void displayWholeList()
{
	for(int i=0;i<LEVEL;i++)
	{
		//printf("head[%d]: ",i);
		if(head[i]!=NULL)
		{
			traverseList(i);
		}
		//else
		//printf(" NULL\n");
	}	
}




void insertInListHead(node *ptr,int level)
{
	if(head[level]==NULL)
	{
		
		ptr->nxt=NULL;
		ptr->prev=NULL;
		head[level]=ptr;
		tail[level]=head[level];
		
		return;
	}
	ptr->prev=NULL;
	head[level]->prev=ptr;
	ptr->nxt=head[level];
	head[level]=ptr;
	
}

int findLevelForAlloc(int mem)
{
	if(mem>MAX_MEM-sizeof(node))
	{
		return -1;
	}
	int startMem=MAX_MEM;
	int level=0;
	while(startMem-MEM_OFFSET>mem&&level<LEVEL)
	{
		startMem=startMem>>1;
		level++;
	}
	if(startMem-sizeof(node)<=mem)
	{
		startMem=startMem*2;
		level--;
	}
	else
	level--;
	
	return level;
}

void *findStartOfHeap(node *ptr)
{
	mallocArena *tmp=arenaHead;
	while(tmp!=NULL)
	{
		if((node*)tmp->startOfHeap<ptr&&(node*)tmp->endOfHeap>ptr)
		return tmp->startOfHeap;
		tmp=tmp->nxt;
	}
	return NULL;
}

node *findFreeBlock(int level)
{
	node *p;
	if(head[level]!=NULL)
	{
		p=head[level];
		if(head[level]==tail[level])
		{
			head[level]=NULL;
			tail[level]=NULL;
		
		}
		else
		{
			head[level]=head[level]->nxt;
			head[level]->prev=NULL;
		}
		//p->nxt=NULL;

		return p;
	}
	
	return NULL;
}

void insertInListTail(node *ptr,int level)
{
	ptr->nxt=NULL;
	ptr->prev=NULL;
			
	if(head[level]==NULL)
	{
		head[level]=ptr;
		tail[level]=ptr;
		
	}
	else
	{
		tail[level]->nxt=ptr;
		ptr->prev=tail[level];
		tail[level]=ptr;	
	}
}


node *findAndBreakBlock(int level)
{
	node *block;
	block=findFreeBlock(level);
	if(block)
	{
		arenaStats.usedBlocks++;
		arenaStats.freeBlocks--;
		block->isFree=0;
		return block;
	}
	int blevel=level-1;
	while(blevel>=0)
	{
		
		block=findFreeBlock(blevel);
		node *blk1,*blk2;
		if(block!=NULL)
		{
			arenaStats.totBlocks++;
			arenaStats.freeBlocks++;
			blk1=block;
			blk2=(node*)((ull)block+(MAX_MEM>>(blevel+1)));
			blk1->level=block->level+1;
			blk1->isFree=1;
			blk2->isFree=1;
			blk2->level=blk1->level;
			insertInListTail(blk1,blevel+1);
			blevel++;
			while(blevel<level)
			{
				arenaStats.totBlocks++;
				arenaStats.freeBlocks++;
				blk1=blk2;
				blk2=(node*)((ull)blk1+(MAX_MEM>>(blevel+1)));
				blk1->level++;
				blk2->level=blk1->level;
				blk1->isFree=1;
				blk2->isFree=1;
				insertInListTail(blk1,blevel+1);
				blevel++;
					
			}
			arenaStats.freeBlocks--;
			arenaStats.usedBlocks++;
			blk2->isFree=0;
			return blk2;
		}
		blevel--;
	}
	return NULL;
}
void insertNewHeap(mallocArena *ptr)
{
	if(arenaHead==NULL)
	{
		ptr->nxt=NULL;
		ptr->prev=NULL;
		arenaHead=ptr;
		arenaTail=ptr;
		arenaTail->startOfHeap=(void*)((ull)ptr+sizeof(mallocArena));
		arenaTail->endOfHeap=(void*)((ull)ptr+MAX_MEM);

	}
	else
	{
		arenaTail->nxt=ptr;
		ptr->prev=arenaTail;
		arenaTail=ptr;
		arenaTail->nxt=NULL;
		arenaTail->startOfHeap=(void*)((ull)ptr+sizeof(mallocArena));
		arenaTail->endOfHeap=(void*)((ull)ptr+MAX_MEM);
	}
}

void requestMoreMem()
{
	mallocArena *newHeap=mmap(NULL,MAX_MEM+sizeof(mallocArena),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
	if(newHeap==MAP_FAILED)
	{
		
		//perror("mmap() failed\n");
		//exit(-1);

	}
	insertNewHeap(newHeap);
	node *ptr=newHeap->startOfHeap;
	ptr->level=0;
	ptr->isFree=1;
	insertInListHead(ptr,0);
}

int getApproxPages(size_t memSize)
{
	int n=0;
	n=memSize/sysconf(_SC_PAGESIZE);
	if(memSize%sysconf(_SC_PAGESIZE))
	{
		n++;
	}
	return n;
}

void addMemPageInLargeChunk(node *ptr)
{
	int lIndex=ptr->level-MIN_LARGE_CHUNK;
	if(lIndex>=LARGE_CHUNKS)
	lIndex=LARGE_CHUNKS-1;
	if(largeChunksListHead[lIndex]==NULL)
	{
		ptr->isFree=1;
		ptr->nxt=NULL;
		ptr->prev=NULL;
		largeChunksListHead[lIndex]=ptr;
		largeChunksListTail[lIndex]=largeChunksListHead[lIndex];
		return;
	}
	ptr->nxt=largeChunksListHead[lIndex];
	largeChunksListHead[lIndex]->prev=ptr;
	largeChunksListHead[lIndex]=ptr;
	ptr->prev=NULL;
	
}
node *removeLargeChunk(size_t memSize)
{
	int totReqPage;
	totReqPage=getApproxPages(memSize+MEM_OFFSET);
	int index;
	index=totReqPage-MIN_LARGE_CHUNK;
	node *retPtr;
	if(index>=LARGE_CHUNKS)
	{
		node *tmp=largeChunksListHead[LARGE_CHUNKS-1];
		while(tmp!=NULL)
		{
			if(tmp->level>=totReqPage)
			{
				if(largeChunksListHead[index]!=NULL)
				{
					if(largeChunksListHead[index]==largeChunksListTail[index])
					{
						retPtr=largeChunksListHead[index];
						largeChunksListHead[index]=NULL;
						largeChunksListTail[index]=NULL;
						return retPtr;
					}
					retPtr=largeChunksListHead[index];
					largeChunksListHead[index]=largeChunksListHead[index]->prev;
					largeChunksListHead[index]->prev=NULL;
					return retPtr;
				}
				
			}
			tmp=tmp->nxt;
		}
		return NULL;
	}
	if(largeChunksListHead[index]!=NULL)
	{
		if(largeChunksListHead[index]==largeChunksListTail[index])
		{
			retPtr=largeChunksListHead[index];
			largeChunksListHead[index]=NULL;
			largeChunksListTail[index]=NULL;
			return retPtr;
		}
		retPtr=largeChunksListHead[index];
		largeChunksListHead[index]=largeChunksListHead[index]->prev;
		largeChunksListHead[index]->prev=NULL;
		return retPtr;
	}	
	return NULL;
}

node *addLargeChunk(size_t memSize)
{
	int totReqPage;
	totReqPage=getApproxPages(memSize+MEM_OFFSET);
	node *newBlock=mmap(NULL,totReqPage*sysconf(_SC_PAGESIZE),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
	if(newBlock==MAP_FAILED){
		return NULL;
	}
	newBlock->level=totReqPage;
//	addMemPageInLargeChunk(newBlock);
	return newBlock;
}
void *malloc(size_t memSize)
{
	if(arenaHead==NULL){
		initBuddy();
	}
	void *retBlock;
	if(memSize<=MAX_MEM-MEM_OFFSET){
		arenaStats.totalMemReq++;
		int l;
		if(memSize<=0)
		return NULL;

		l=findLevelForAlloc(memSize);
		//printf("level: %d\n",l);
		
		
		if(l==-1)		
			return NULL;
		pthread_mutex_lock(&lock);	//lock	
		retBlock=findAndBreakBlock(l);
		pthread_mutex_unlock(&lock);//unlock
	
		if(retBlock!=NULL){
			return (void*)((ull)retBlock+(ull)MEM_OFFSET);
		}
		pthread_mutex_lock(&lock);	//lock	

		requestMoreMem();
		//printf("heapAdded\n");
		//displayWholeList();
		retBlock=findAndBreakBlock(l);
		//displayWholeList();
		pthread_mutex_unlock(&lock);//unlock

		if(retBlock!=NULL)
			return (void*)((ull)retBlock+(ull)MEM_OFFSET);

		return NULL;
	}
	else{
			retBlock=removeLargeChunk(memSize);
		if(retBlock!=NULL)
			return (void*)((ull)retBlock+(ull)MEM_OFFSET);
		retBlock=addLargeChunk(memSize);
		if(retBlock!=NULL)
			return (void*)((ull)retBlock+(ull)MEM_OFFSET);
		
		return NULL;
	}
	
}
node *findBuddy(node* p,int level)
{
	
	node *tmp;
	ull heapStart=(ull)findStartOfHeap(p);
	if(heapStart==0)
	return NULL;
	if(level==0)
	return NULL;
	if((((ull)p-heapStart)/(MAX_MEM>>level))%2==0)
	{
		tmp=(node*)((ull)p+(MAX_MEM>>level));
		//printf("Right child:%p\n",tmp);
		
		if(tmp->isFree==1)
		return tmp;
	}
	else
	{
		tmp=(node*)((ull)p-(MAX_MEM>>level));
		//printf("Left child:%p\n",tmp);
		
		if(tmp->isFree==1)
		return tmp;
	}
	return NULL;
}

node *detachBlock(node *p,int level)
{
	if(head[level]==tail[level]&&p==head[level])
	{
		head[level]=tail[level]=NULL;
		return p;
	}
	if(head[level]==p)
	{
		head[level]=head[level]->nxt;
		head[level]->prev=NULL;
		return p;
	}
	if(tail[level]==p)
	{
		tail[level]=tail[level]->prev;
		tail[level]->nxt=NULL;
		return p;
	}
	p->nxt->prev=p->prev;
	p->prev->nxt=p->nxt;
	return p;
	
}


void coalesceBuddies(node *p)
{
	int level=p->level;
//	printf("Level(Free): %d\n",level);
//	detachBlock(p,level);
	arenaStats.freeBlocks++;
	while(level>=0)
	{
		node *p1=findBuddy(p,level);
		if(p1!=NULL)
		{
	//		printf("Buddy found\n");
			detachBlock(p1,level);
			arenaStats.freeBlocks--;
			arenaStats.totBlocks--;
			if((ull)p1<(ull)p)
			{
				p1->level=p1->level-1;
			}
			else
			{
				p->level=p->level-1;
				p1=p;
			}
		}
		else
		{
			insertInListHead(p,level);
			return;
		}
			
		p=p1;
		level--;
		
	}
	
}

void unmapLargeChunk(node *p)
{
	if(munmap(p,p->level+MEM_OFFSET)==-1)
	{
		//printf("Unmap error");
	}
}
void free(void *fptr)
{
	arenaStats.totalFreeMemReq++;
	node *p=(node*)((ull)fptr-MEM_OFFSET);
	if(p->level>LEVEL)
	{
		addMemPageInLargeChunk(p);
		return ;
	}
	p->isFree=1;
	//printBlock(p);
	coalesceBuddies(p);
}


void *realloc(void *ptr,size_t memSize)
{
	node *tmp;
	if(ptr==NULL)
		return malloc(memSize);
	if(memSize==0){
		free(ptr);
		return NULL;
	}
	tmp=(void*)((ull)ptr-MEM_OFFSET);
	int level;
	level=findLevelForAlloc(memSize);
	if(level==-1)
		return NULL;	
	if(tmp->level==level){
		return ptr;
	}
	else
	{
		void *retPtr=malloc(memSize);
		if(retPtr!=NULL)
		{
			memcpy(retPtr,ptr,memSize);
			free(ptr);
			return retPtr;
		}
		return NULL;
	}
}

void *calloc(size_t nMem,size_t size)
{
	if(nMem==0||size==0)
	return NULL;
	
	void *retPtr;
	retPtr=malloc(nMem*size);
	if(retPtr!=NULL)
	{
		memset(retPtr,0,nMem*size);
		return retPtr;
	}
	return NULL;
}

