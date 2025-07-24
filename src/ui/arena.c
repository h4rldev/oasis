#include <string.h>

#include <oasis/ui/arena.h>

// Initialize the global static arena
Arena arena = {0};

void create_arena(size_t size) {
  arena.size = size;
  arena.offset = 0;
}

void *alloc_arena(size_t size, void *content) {
  if (arena.offset + size >= arena.size) {
    // Arena is full
    return NULL;
  }

  void *result = (uint8_t *)arena.memory + arena.offset;
  arena.offset += size;

  if (content != NULL) {
    memcpy(result, content, size);
  }

  return result;
}

void destroy_arena(void) {
  arena.size = 0;
  arena.offset = 0;
  arena.memory = NULL;
}
