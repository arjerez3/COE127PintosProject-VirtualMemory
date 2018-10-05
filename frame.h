#ifdef VM_FRAME_H
#define VM_FRAME_H

#include "threads/synch.h"	//For lock mechanism

struct frame
{
	void* base;		//Base address of the frame
	struct lock lock;	//Protection for frame
	struct page* page;	//Reference to its page
}

void frame_init(void);	//Accept a void pointer

struct frame * frame_alloc_and_lock(struct page *);


#endif
