#ifndef OASIS_UI_ARENA_H
#define OASIS_UI_ARENA_H

#include <clay.h>
#include <oasis/utils.h>

/**
 * The global static arena.
 * @param offset: The offset of the next allocation.
 * @param memory: The memory.
 */

typedef struct {
  uintptr_t offset;
  uintptr_t size;
  void *memory;
} Arena;

/**
 * Creates the global static arena.
 * @param size: The size of the arena.
 */

void create_arena(size_t size);

/**
 * Allocates a known type of memory.
 * @param size: The size of the memory to allocate.
 * @param content: The content of the memory to allocate.
 * @return The allocated memory, or NULL if it failed.
 */
void *alloc_arena(size_t size, void *content);

/**
 * Destroys the global arena.
 */

void destroy_arena(void);

#endif
