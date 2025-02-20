#ifndef MEMORY_H
#define MEMORY_H

#include <inttypes.h>
#include <stdlib.h>

#include "../VortexConfig.h"

#if DEBUG_ALLOCATIONS == 1

// monitored allocation routines
#define vmalloc(size) _vmalloc(size)
#define vcalloc(size, amount) _vcalloc(size, amount)
#define vrealloc(ptr, size) _vrealloc(ptr, size)
#define vfree(ptr) _vfree(ptr)

// Vortex allocation functions
void *_vmalloc(uint32_t size);
void *_vcalloc(uint32_t size, uint32_t amount);
void *_vrealloc(void *ptr, uint32_t size);
void _vfree(void *ptr);

// memory used by regular code
uint32_t cur_memory_usage();
// memory used by the memory tracker itself
uint32_t cur_memory_usage_background();
// memory used by everything
uint32_t cur_memory_usage_total();

#else // if DEBUG_ALLOCATIONS

#define vmalloc(size) malloc(size)
#define vcalloc(size, amount) calloc(size, amount)
#define vrealloc(ptr, size) realloc(ptr, size)
#define vfree(ptr) free(ptr)

#endif

#ifndef VORTEX_LIB
//void *operator new  (size_t size);
//void *operator new[](size_t size);
//void  operator delete  (void *ptr);
//void  operator delete[](void *ptr);
//void *operator new  (size_t size, void *ptr) noexcept;
//void *operator new[](size_t size, void *ptr) noexcept;
//void  operator delete  (void *ptr, size_t size) noexcept;
//void  operator delete[](void *ptr, size_t size) noexcept;
//void *operator new  (size_t size, std::align_val_t al);
//void *operator new[](size_t size, std::align_val_t al);
//void  operator delete  (void *ptr, std::align_val_t al) noexcept;
//void  operator delete[](void *ptr, std::align_val_t al) noexcept;
//void  operator delete  (void *ptr, size_t size, std::align_val_t al) noexcept;
//void  operator delete[](void *ptr, size_t size, std::align_val_t al) noexcept;
#endif

#endif
