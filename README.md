# mallocLibrary
A malloc library with buddy implementation.
This library is implementation of buddy allocation. I have used a doubly linked list for every level of buddy system.
And for allocations greater than biggest buddy block, it uses large fastbins to allocate memory which finds block by first fit method.
malloc is thread safe and can be used in multi-threaded programs.
Every buddy has a header where it keeps the information about level(for buddies) or size(for large fastbins) and two pointers of doublylinked list. More memory could have been saved by keeping header size small but at the cost of alignment. 
When any buddy is removed from the list, two pointers are used up as user data and only level or size is kept as metadata to save memory.
An arena is allocated for every thread. It doens't uses heap memory, it maps(using mmap()) new arenas for every thread.
Returned addresses are 8 byte memory aligned.
Malloc always asks for new memory using mmap in multiples of pages. 
This library can be improved a lot by following additions which i plan for next version:
1. Using a single centralised heap which provides memory to arenas per threads as required or requested.
2. Using slab allocator for small allocations which will reduce internal fragmentation and will speed up allocations for certain     programs as generally they request small memory. 
3. Instead of memory header for small allocations, an array of single bit could have been used as to keep informaton about free and used memory and depending upon the location of bit, the particular address of the memory block would hav been calculated and similarly using block address respective bit position can be calculated, This approach would have saved a lot of memory by minimising internal fragmentation but at the cost of speed as calculation of bit postion and memory address takes some time but could have been a good approach. 
