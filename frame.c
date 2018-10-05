#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"


static struct frame *frame_table;	//Fixed array of frames
static size_t frame_size;		//Size of frame in bytes

static struct lock scan_lock;		//Lock for scanning

//Initialize a frame
void frame_init()
{
	void *base;		//Iterative address for each frame

	lock_init(&scan_lock);		//Initialize lock

	frame_table = malloc(sizeof *frames * init_ram_pages);
	
	//If frame table is not allocated
	if(frame_table == NULL)
		PANIC("Out of memory allocating page frames");

	while((base = palloc_get_page(PAL_USER)) != NULL)
	{
		//Create a frame instance
		struct frame *frame_inst = &frame_table[frame_size];
		++frame_size;
		lock_init(&frame_inst->lock);
		frame_inst->base = base;
		frame_inst->page = NULL;
	}
	
}

//Allocate a frame for a given page
try_frame_alloc_and_lock(struct page * page)
{
	size_t i;
	
	lock_acquire(&scan_lock);

	//Find a free frame by iterating through the frame table
	for(i = 0; i < frame_size; i++)
	{
		struct frame * frame_inst = &frame_table[i];
		//If the frame is locked, skip to next frame
		if(!lock_try_acquire(&frame_inst->lock))
			continue;
		//If the frame instance has no page reference
		if(frame_inst->page == NULL)
		{
			frame_inst->page = page;
			lock_release(&scan_lock);
			return frame_inst;
		}
		lock_release(&frame_inst->lock);
	}

	//Implement frame eviction here
}
