#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>

/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */

struct memoryList
{
    // doubly-linked list
    struct memoryList *last;
    struct memoryList *next;

    int size;   // How many bytes in this block?
    char alloc; // 1 if this block is allocated,
                // 0 if this block is free.
    void *ptr;  // location of block in memory pool.
};

struct memoryList *firstBlock(size_t requested);
struct memoryList *bestBlock(size_t requested);
struct memoryList *worstBlock(size_t requested);
struct memoryList *nextBlock(size_t requested);

strategies myStrategy = NotSet; // Current strategy

size_t mySize;
void *myMemory = NULL;

static struct memoryList *head;
static struct memoryList *next;
struct memoryList *trav; // memory list traversal pointer

/* initmem must be called prior to mymalloc and myfree.

   initmem may be called more than once in a given exeuction;
   when this occurs, all memory you previously malloc'ed  *must* be freed,
   including any existing bookkeeping data.

   strategy must be one of the following:
        - "best" (best-fit)
        - "worst" (worst-fit)
        - "first" (first-fit)
        - "next" (next-fit)
   sz specifies the number of bytes that will be available, in total, for all mymalloc requests.
*/

void initmem(strategies strategy, size_t sz)
{
    myStrategy = strategy;

    /* all implementations will need an actual block of memory to use */
    mySize = sz;

    if (myMemory != NULL)
        free(myMemory); /* in case this is not the first time initmem2 is called */

    // Release any other memory you were using for bookkeeping when doing a re-initialization!

    // check if there is at least one block allocated in memory
    if (head != NULL)
    {
        // traverse the doubly linked list
        for (trav = head; trav->next != NULL; trav = trav->next)
        {
            // free previous node in memory
            free(trav->last);
        }
        // finally free the traversal pointer
        free(trav);
    }

    myMemory = malloc(sz);

    // Initialize memory management structure.

    // init first node of memory, from https://github.com/ArmandasRokas/dtu_notes/wiki/ass3_manual
    head = (struct memoryList *)malloc(sizeof(struct memoryList));
    head->last = head;
    head->next = head;
    head->size = sz;      // initialy the first block size is equals to the memory pool size.
    head->alloc = 0;      // not allocated
    head->ptr = myMemory; // points to the same memory adress as the memory pool
    next = head;          // only used for next fit
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1
 */

void *mymalloc(size_t requested)
{
    assert((int)myStrategy > 0);

    struct memoryList *memBlock = NULL;
    switch (myStrategy)
    {
    case First:
        memBlock = firstBlock(requested);
        break;
    case Best:
        memBlock = bestBlock(requested);
        break;
    case Worst:
        memBlock = worstBlock(requested);
        break;
    case Next:
        memBlock = nextBlock(requested);
        break;
    default:
        // no strategy
        return NULL;
    }

    // no valid blocks
    if (!memBlock)
    {
        return NULL;
    }

    if (memBlock->size > requested)
    {
        struct memoryList *remainder = malloc(sizeof(struct memoryList));

        // insert remainder into the memory
        remainder->next = memBlock->next;
        // remainder->next->last = remainder;
        remainder->last = memBlock;
        memBlock->next = remainder;

        // divide memory
        remainder->size = memBlock->size - requested;
        remainder->alloc = 0;
        // TODO illegal pointer arithmetic?
        remainder->ptr = memBlock->ptr + requested;
        memBlock->size = requested;
        next = remainder;
    }
    else
    {
        next = memBlock->next;
    }

    memBlock->alloc = 1;

    // pointer is returned to block
    return memBlock->ptr;
}

// Find the first block of memory larger than the requested size which is available
struct memoryList *firstBlock(size_t requested)
{
    // find block
    struct memoryList *index = head;
    do
    {
        if (!(index->alloc) && index->size >= requested)
        {
            return index;
        }
    } while ((index = index->next) != head);

    // no block
    return NULL;
}

// Find the smallest block larger than the requested size which is not allocated
struct memoryList *bestBlock(size_t requested)
{
    // find most optimally sized block
    struct memoryList *index = head, *min = NULL;
    do
    {
        if (!(index->alloc) && index->size >= requested && (!min || index->size < min->size))
        {
            min = index;
        }
    } while ((index = index->next) != head);

    return min;
}

// Find the largest block larger than the requested size which is not allocated
struct memoryList *worstBlock(size_t requested)
{
    // find biggest block
    struct memoryList *index = head, *max = NULL;
    do
    {
        if (!(index->alloc) && (!max || index->size > max->size))
        {
            max = index;
        }
    } while ((index = index->next) != head);

    // return found block if big enough
    if (max->size >= requested)
    {
        return max;
    }
    else
    {
        return NULL;
    }
}

/* Find the first suitable block after the last block allocated. */
struct memoryList *nextBlock(size_t requested)
{
    // find next big enough block
    struct memoryList *start = next;
    do
    {
        if (!(next->alloc) && next->size >= requested)
        {
            return next;
        }
    } while ((next = next->next) != start);

    // no block
    return NULL;
}

/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void *block)
{
    // iterate over memory, find target block container
    struct memoryList *cont = head;
    do
    {
        if (cont->ptr == block)
        {
            break;
        }
    } while ((cont = cont->next) != head);

    cont->alloc = 0;

    // reduce to a single block if prev is free
    if (cont != head && cont->last->alloc == 0)
    {
        struct memoryList *prev = cont->last;
        prev->next = cont->next;
        prev->next->last = prev;
        prev->size += cont->size;

        if (next == cont)
        {
            next = prev;
        }

        free(cont);
        cont = prev;
    }

    // reduce to single block if next is free
    if (cont->next != head && !(cont->next->alloc))
    {
        struct memoryList *second = cont->next;
        cont->next = second->next;
        cont->next->last = cont;
        cont->size += second->size;

        if (next == second)
        {
            next = cont;
        }

        free(second);
    }
}

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when refered to "memory" here, it is meant that the
 * memory pool this module manages via initmem/mymalloc/myfree.
 */

/* Get the number of contiguous areas of free space in memory. */
int mem_holes()
{
    // TODO foreach block in memory, sum the unallocated spaces and return total
    return 0;
}

/* Get the number of bytes allocated */
int mem_allocated()
{
    // TODO foreach block in memory, sum the allocated bytes and return total
    return 0;
}

/* Number of non-allocated bytes */
int mem_free()
{
    // TODO foreach block in memory, sum the unallocated bytes and return total
    return 0;
}

/* Number of bytes in the largest contiguous area of unallocated memory */
int mem_largest_free()
{
    return 0;
}

/* Number of free blocks smaller than "size" bytes. */
int mem_small_free(int size)
{
    // TODO foreach block in memory, sum the amount of blocks smaller than *size* bytes and return count.
    return 0;
}

char mem_is_alloc(void *ptr)
{
    return 0;
}

/*
 * Feel free to use these functions, but do not modify them.
 * The test code uses them, but you may find them useful.
 */

// Returns a pointer to the memory pool.
void *mem_pool()
{
    return myMemory;
}

// Returns the total number of bytes in the memory pool. */
int mem_total()
{
    return mySize;
}

// Get string name for a strategy.
char *strategy_name(strategies strategy)
{
    switch (strategy)
    {
    case Best:
        return "best";
    case Worst:
        return "worst";
    case First:
        return "first";
    case Next:
        return "next";
    default:
        return "unknown";
    }
}

// Get strategy from name.
strategies strategyFromString(char *strategy)
{
    if (!strcmp(strategy, "best"))
    {
        return Best;
    }
    else if (!strcmp(strategy, "worst"))
    {
        return Worst;
    }
    else if (!strcmp(strategy, "first"))
    {
        return First;
    }
    else if (!strcmp(strategy, "next"))
    {
        return Next;
    }
    else
    {
        return 0;
    }
}

/*
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */

/* Use this function to print out the current contents of memory. */
void print_memory()
{
    printf("Memory List {\n");
    /* Iterate over memory list */
    struct memoryList *index = head;
    do
    {
        printf("\tBlock %p,\tsize %d,\t%s\n",
               index->ptr,
               index->size,
               (index->alloc ? "[ALLOCATED]" : "[FREE]"));
    } while ((index = index->next) != head);
    printf("}\n");
}

/* Use this function to track memory allocation performance.
 * This function does not depend on your implementation,
 * but on the functions you wrote above.
 */
void print_memory_status()
{
    printf("%d out of %d bytes allocated.\n", mem_allocated(), mem_total());
    printf("%d bytes are free in %d holes; maximum allocatable block is %d bytes.\n", mem_free(), mem_holes(), mem_largest_free());
    printf("Average hole size is %f.\n\n", ((float)mem_free()) / mem_holes());
}

/* Use this function to see what happens when your malloc and free
 * implementations are called.  Run "mem -try <args>" to call this function.
 * We have given you a simple example to start.
 */
void try_mymem(int argc, char *argv)
{
    strategies strat;
    void *a, *b, *c, *d, *e;
    if (argc > 1)
        strat = strategyFromString(argv);
    else
        strat = First;

    /* A simple example.
       Each algorithm should produce a different layout. */

    initmem(strat, 500);

    a = mymalloc(100);
    b = mymalloc(100);
    c = mymalloc(100);

    print_memory();

    myfree(b);

    print_memory();

    d = mymalloc(50);

    print_memory();

    myfree(a);

    e = mymalloc(25);

    print_memory();

    print_memory();
    print_memory_status();
}

int main()
{
    try_mymem(2, "next");
    return 0;
}