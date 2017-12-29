# mallocLibrary
A malloc library with buddy implementation.
This library is implementation of buddy allocation. I have used a doubly linked list for every level of buddy system.
And for allocations greater than biggest buddy block, it uses large fastbins to allocate memory which finds block by first fit method.
malloc is thread safe and can be used in multi-threaded programs. 
An arena is allocated for every thread. It doens't uses heap memory, it maps(using mmap()) new arenas for every thread.
Returned addresses are 8 byte memory aligned.
