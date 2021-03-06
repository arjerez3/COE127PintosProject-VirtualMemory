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
struct frame * try_frame_alloc_and_lock(struct page * page)
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

	for (i = 0; i < frame_cnt * 2; i++) 
    {
      /* Get a frame. */
      struct frame *f = &frames[hand];
      if (++hand >= frame_cnt)
        hand = 0;

      if (!lock_try_acquire (&f->lock))
        continue;

      if (f->page == NULL) 
        {
          f->page = page;
          lock_release (&scan_lock);
          return f;
        } 

      if (page_accessed_recently (f->page)) 
        {
          lock_release (&f->lock);
          continue;
        }
          
      lock_release (&scan_lock);
      
      /* Evict this frame. */
      if (!page_out (f->page))
        {
          lock_release (&f->lock);
          return NULL;
        }

      f->page = page;
      return f;
    }

  lock_release (&scan_lock);
  return NULL;
}

//Allocate and lock a frame given a page
struct frame * frame_alloc_and_lock(struct page * page)
{
	size_t try;

	//After three tries
	for(try = 0; try < 3; try++)
	{
		//Try to allocate a frame for the page
		struct frame * frame_inst = try_frame_alloc_and_lock(page);
		//If the frame is not locked
		if(frame_inst != NULL)
		{
			ASSERT(lock_held_by_current_thread(&frame_inst->lock));
			return frame_inst;
		}
		timer_msleep(1000);
	}
	
	//If no frame is allocated, return null pointer
	return NULL;
}

//Lock a frame
void frame_lock(struct page * p)
{
	//Indicate the frame referred by the given page
	struct frame * frame_inst = p->frame;
	//If there is a frame	
	if(frame_inst != NULL)
	{
		lock_acquire(&frame_inst->lock);

		if(frame_inst != p->frame)
		{
			//Free the frame
			lock_release(&frame_inst->lock);
			ASSERT(p->frame == NULL)
		}
	}
}


