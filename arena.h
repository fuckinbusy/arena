// Copyright 2022 Alexey Kutepov <reximkut@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef ARENA_H_
#define ARENA_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t  arena_size_t;
#define ARENA_SIZE_FMT "%llu"
typedef uintptr_t arena_ptr_t;

#define _ARENA_PLATFORM_LIBC  0x0
#define _ARENA_PLATFORM_WIN32 0x2
#define _ARENA_PLATFORM_WIN64 0x4
#define _ARENA_PLATFORM_UNIX  0x8

#ifdef __GNUC__
    #define _ARENA_FORCE_INLINE static inline __attribute__((always_inline))
    #define _ARENA_PREFETCH(addr) __builtin_prefetch((addr))
#else
    #define _ARENA_FORCE_INLINE static inline
    #define _ARENA_PREFETCH(...)
#endif

#ifndef ARENA_PLATFORM
    #if defined(_WIN32)
        #define ARENA_PLATFORM _ARENA_PLATFORM_WIN32
    #elif defined(_WIN64)
        #define ARENA_PLATFORM _ARENA_PLATFORM_WIN64
    #elif defined(__unix__) || defined(__linux__)
        #define ARENA_PLATFORM _ARENA_PLATFORM_UNIX
    #else
        #define ARENA_PLATFORM _ARENA_PLATFORM_LIBC
    #endif
#endif 

#if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOMINMAX
    #define NOUSER
    #define NOSERVICE
    #define NOMCX
    #define NOSOUND
    #define NOCOMM
    #define NORPC
    #include <windows.h>
#elif ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
    #include <unistd.h>
    #include <sys/mman.h>
#else
    #error("Undefined platform")
#endif

#ifndef ARENA_NOMINAX
    #define ARENA_MAX(a, b) ((a) - (((a) - (b)) & (((a) - (b)) >> 31))) // damn
    #define ARENA_MIN(a, b) ((b) + (((a) - (b)) & (((a) - (b)) >> 31)))
#endif

#define ARENA_U64_MAX  0xffffffffffffffffull
#define ARENA_U32_MAX  0xffffffffu
#define ARENA_SIZE_MAX SIZE_MAX
#define ARENA_PTR_MAX  MAXINT_PTR

#ifndef ARENA_CACHELINE_SIZE
#define ARENA_CACHELINE_SIZE (uint32_t)64
#endif

#define _ARENA_POISON_ALLOC         0xCD   // arena memory poisoning value after allocating memory from arena
#define _ARENA_POISON_RESET         0xDD   // arena memory poisoning value after resetting arena
#define ARENA_PAGE_ALIGN_THRESHOLD 0x2000  // used to identify when to switch to platform specific allocation 
#define ARENA_PAGE_DEFAULT_SIZE    0x1000  // for libc universal platform 

typedef enum ArenaGrowthContract : uint32_t {
    ARENA_GROWTH_CONTRACT_FIXED       = 0,
    ARENA_GROWTH_CONTRACT_REALLOC     = 2,
    ARENA_GROWTH_CONTRACT_CHUNKY      = 4,
} ArenaGrowthContract;

typedef enum ArenaGrowthFactor : uint32_t {
    ARENA_GROWTH_FACTOR_NONE        = 0x0,    // none =D
    ARENA_GROWTH_FACTOR_REALLOC_2X  = 0x2,    // for mul operation
    ARENA_GROWTH_FACTOR_REALLOC_4X  = 0x4,    // for mul operation
    ARENA_GROWTH_FACTOR_REALLOC_8X  = 0x8,    // for mul operation
    ARENA_GROWTH_FACTOR_CHUNKY_512B  = 0x200,
    ARENA_GROWTH_FACTOR_CHUNKY_1KB   = 0x400,
    ARENA_GROWTH_FACTOR_CHUNKY_2KB   = 0x800,
    ARENA_GROWTH_FACTOR_CHUNKY_4KB   = 0x1000,
    ARENA_GROWTH_FACTOR_CHUNKY_8KB   = 0x2000,
    ARENA_GROWTH_FACTOR_CHUNKY_16KB  = 0x4000,
    ARENA_GROWTH_FACTOR_CHUNKY_32KB  = 0x8000,
    ARENA_GROWTH_FACTOR_CHUNKY_64KB  = 0x10000,
    ARENA_GROWTH_FACTOR_CHUNKY_128KB = 0x20000,
    ARENA_GROWTH_FACTOR_CHUNKY_256KB = 0x40000,
    ARENA_GROWTH_FACTOR_CHUNKY_512KB = 0x80000,
    ARENA_GROWTH_FACTOR_CHUNKY_1MB   = 0x100000,
    ARENA_GROWTH_FACTOR_CHUNKY_2MB   = 0x200000,
    ARENA_GROWTH_FACTOR_CHUNKY_MAX   = ARENA_GROWTH_FACTOR_CHUNKY_2MB,
    ARENA_GROWTH_FACTOR_CHUNKY_MIN   = ARENA_GROWTH_FACTOR_CHUNKY_512B,
} ArenaGrowthFactor;

#define ARENA_CAPACITY_CHOOSE_FOR_ME_PLS 0 
#define ARENA_CAPACITY_512B    (arena_size_t)0x200
#define ARENA_CAPACITY_1KB     (arena_size_t)0x400
#define ARENA_CAPACITY_2KB     (arena_size_t)0x800
#define ARENA_CAPACITY_4KB     (arena_size_t)0x1000
#define ARENA_CAPACITY_8KB     (arena_size_t)0x2000
#define ARENA_CAPACITY_16KB    (arena_size_t)0x4000
#define ARENA_CAPACITY_32KB    (arena_size_t)0x8000
#define ARENA_CAPACITY_64KB    (arena_size_t)0x10000
#define ARENA_CAPACITY_128KB   (arena_size_t)0x20000
#define ARENA_CAPACITY_256KB   (arena_size_t)0x40000
#define ARENA_CAPACITY_512KB   (arena_size_t)0x80000
#define ARENA_CAPACITY_1MB     (arena_size_t)0x100000
#define ARENA_CAPACITY_2MB     (arena_size_t)0x200000
#define ARENA_CAPACITY_4MB     (arena_size_t)0x400000
#define ARENA_CAPACITY_8MB     (arena_size_t)0x800000
#define ARENA_CAPACITY_16MB    (arena_size_t)0x1000000
#define ARENA_CAPACITY_32MB    (arena_size_t)0x2000000
#define ARENA_CAPACITY_64MB    (arena_size_t)0x4000000
#define ARENA_CAPACITY_128MB   (arena_size_t)0x8000000
#define ARENA_CAPACITY_256MB   (arena_size_t)0x10000000
#define ARENA_CAPACITY_512MB   (arena_size_t)0x20000000
#define ARENA_CAPACITY_1GB     (arena_size_t)0x40000000
#define ARENA_CAPACITY_2GB     (arena_size_t)0x80000000
#define ARENA_CAPACITY_4GB     (arena_size_t)0x100000000
#define ARENA_CAPACITY_8GB     (arena_size_t)0x200000000
#define ARENA_CAPACITY_DEFAULT ARENA_CAPACITY_4KB
#define ARENA_CAPACITY_MAX     ARENA_CAPACITY_8GB
#define ARENA_CAPACITY_MIN     ARENA_CAPACITY_512B

#define ARENA_ALLOC_TYPE_SMALL 0x0 // arena is allocated as an array of bytes
#define ARENA_ALLOC_TYPE_BIG   0x1 // arena is allocated as memory pages

typedef enum ArenaAlignment : uint32_t {
    // Alignment constants
    ARENA_ALIGN_2B      = 2,
    ARENA_ALIGN_4B      = 4,
    ARENA_ALIGN_8B      = 8,
    ARENA_ALIGN_16B     = 16,
    ARENA_ALIGN_32B     = 32,
    ARENA_ALIGN_64B     = 64,
    ARENA_ALIGN_128B    = 128,
    ARENA_ALIGN_256B    = 256,
    ARENA_ALIGN_512B    = 512,
    ARENA_ALIGN_DEFAULT = ARENA_ALIGN_16B,
    
    // SIMD support
    ARENA_ALIGN_SIMD_MMX       = 8,
    ARENA_ALIGN_SIMD_SSE       = 16,
    ARENA_ALIGN_SIMD_AVX       = 32,
    ARENA_ALIGN_SIMD_CACHELINE = 64, // x86/ARM
    ARENA_ALIGN_SIMD_AVX512    = 512,

    // Cache line sized alignment
    ARENA_ALIGN_CACHELINE = ARENA_CACHELINE_SIZE,
} ArenaAlignment;

static inline const char *arena_capacity_str(size_t capacity)
{
    switch (capacity) {
        case ARENA_CAPACITY_1KB:       return "1KB";
        case ARENA_CAPACITY_2KB:       return "2KB";
        case ARENA_CAPACITY_4KB:       return "4KB";
        case ARENA_CAPACITY_8KB:       return "8KB";
        case ARENA_CAPACITY_16KB:      return "16KB";
        case ARENA_CAPACITY_32KB:      return "32KB";
        case ARENA_CAPACITY_64KB:      return "64KB";
        case ARENA_CAPACITY_128KB:     return "128KB";
        case ARENA_CAPACITY_256KB:     return "256KB";
        case ARENA_CAPACITY_512KB:     return "512KB";
        case ARENA_CAPACITY_1MB:       return "1MB";
        case ARENA_CAPACITY_2MB:       return "2MB";
        case ARENA_CAPACITY_4MB:       return "4MB";
        case ARENA_CAPACITY_8MB:       return "8MB";
        case ARENA_CAPACITY_16MB:      return "16MB";
        case ARENA_CAPACITY_32MB:      return "32MB";
        case ARENA_CAPACITY_64MB:      return "64MB";
        case ARENA_CAPACITY_128MB:     return "128MB";
        case ARENA_CAPACITY_256MB:     return "256MB";
        case ARENA_CAPACITY_512MB:     return "512MB";
        case ARENA_CAPACITY_1GB:       return "1GB";
        case ARENA_CAPACITY_MAX:       return "8GB"; 
        default:                       return "Custom";
    }
}

static inline const char *arena_platform_str()
{
    switch (ARENA_PLATFORM) {
        case _ARENA_PLATFORM_WIN32: return "WIN32";
        case _ARENA_PLATFORM_WIN64: return "WIN64";
        case _ARENA_PLATFORM_UNIX:  return "UNIX";
        case _ARENA_PLATFORM_LIBC:  return "LIBC";
        default:                    return "UNKNOWN";
    }
}

typedef enum ArenaFlag {
    ARENA_FLAG_NONE              = 0,
    ARENA_FLAG_FILLZEROES        = 1,
    ARENA_FLAG_DEBUG             = 1 << 1,
    ARENA_FLAG_ENFORCE_ALIGNMENT = 1 << 2,
    ARENA_FLAG_RESET_AFTER_GROW  = 1 << 3,
} ArenaFlag;

typedef enum ArenaError {
    ARENA_ERROR_NONE = 0,
    ARENA_ERROR_OOM,
    ARENA_ERROR_INVALID_ALIGNMENT,
} ArenaError;

typedef struct ArenaConfig {
    arena_size_t        capacity;
    arena_size_t        max_capacity;
    ArenaGrowthContract growth_contract;
    size_t              growth_factor;
    ArenaFlag           flags;
} ArenaConfig;

typedef struct ArenaDebugInfo {
    uint8_t      *end;              // address to the end of the last allocated block of memory
    arena_size_t bytes_lost;        // bytes loss due to alignment
    size_t       total_allocations; // obviously number of total allocations
} ArenaDebugInfo;

typedef struct ArenaChunk {
    struct ArenaChunk  *next;
    arena_size_t       capacity;
    arena_size_t       offset;
    uint8_t            base[];
} ArenaChunk;

typedef struct ArenaMemory {
    ArenaChunk   *chunk;    // pointer to chunk
    void         *data;     // pointer to data
    size_t       alignment; // how data is aligned in this memory
    arena_size_t offset;    // offset from the chunk base to the data
    arena_size_t size;      // memory size
    arena_size_t epoch;
} ArenaMemory;

typedef struct ArenaMark {
    ArenaChunk   *chunk;
    arena_size_t offset;
    arena_size_t epoch;
} ArenaMark;

typedef struct Arena {
    arena_size_t        reserved;        // memory reserved for user data (does not include chunk metadata and used for OOM check)
    arena_size_t        max_capacity;    // max arena capacity (machine dependent), it shows how big this arena can be if it can grow
    arena_size_t        growth_factor;   // how fast grows (depends on contract)
    ArenaGrowthContract growth_contract; // arena growth contract (how to grow)
    ArenaFlag           flags;           // arena flags - ARENA_FLAG_...
    ArenaError          error;           // error flag
    uint32_t            alloc_type;      // reflects how arena memory was originally allocated and must not change during arena lifetime
    arena_size_t        epoch;

    ArenaChunk          *head_chunk;
    ArenaChunk          *last_chunk;

    ArenaDebugInfo      debug;
} Arena;

#define ARENA_EMPTY ((Arena){0})

static inline Arena arena_create_ex(ArenaConfig config);
static inline ArenaConfig arena_config_create(arena_size_t capacity, arena_size_t max_capacity, ArenaGrowthContract contract, size_t growth_factor, ArenaFlag flags);
static inline Arena arena_create(arena_size_t capacity);
static inline void arena_destroy(Arena *arena);
static inline ArenaMemory arena_alloc(Arena *arena, arena_size_t size, size_t alignment);
static inline void *arena_alloc_raw(Arena *arena, arena_size_t size, size_t alignment);
static inline ArenaMemory arena_alloc_zero(Arena *arena, arena_size_t size, size_t alignment);
static inline bool arena_reset(Arena *arena);
static inline bool arena_grow(Arena *arena, arena_size_t grow_size);
static inline void *arena_memory_resolve(const Arena *arena, ArenaMemory *memory);
static inline ArenaMark arena_mark(const Arena *arena);
static inline bool arena_restore(Arena *arena, ArenaMark mark, bool poison_memory);

_ARENA_FORCE_INLINE long long arena_abs(long long value);

#ifdef ARENA_USE_STD_STRING // if defined <string.h> functions will be used
    #include <string.h>
    #define arena_memcpy      memcpy
    #define arena_memset      memset
    #define arena_strdup      strdup
    #define arena_strlen      strlen
    #define arena_strlen_fast strlen
#else
    static inline void *arena_memcpy(void *dst, const void *src, size_t count);
    static inline void *arena_memset(void *dst, int value, size_t size);
    static inline char *arena_strdup(Arena *arena, const char *src);
    static inline size_t arena_strlen(const char *str);
    static inline size_t arena_strlen_fast(const char *str);
#endif

#define ARENA_IMPLEMENTATION
#ifdef ARENA_IMPLEMENTATION

#ifdef ARENA_LOGGING
    #include <stdarg.h>
    #include <stdio.h>
    #define ARENA_LOG(fmt, ...) printf("[+] "fmt"\n", ##__VA_ARGS__)
#else
    #define ARENA_LOG(...)
#endif

_ARENA_FORCE_INLINE arena_size_t _arena_sadd(arena_size_t a, arena_size_t b, arena_size_t max)
{
    return (a > (max - b)) ? max : (a + b);
}

_ARENA_FORCE_INLINE arena_size_t _arena_ssub(arena_size_t a, arena_size_t b, arena_size_t min)
{
    return (a < (b + min)) ? min : (a - b);
}

_ARENA_FORCE_INLINE arena_size_t _arena_smul(arena_size_t a, arena_size_t b, arena_size_t max)
{
    return (a > (max / b)) ? max : (a * b);
}

_ARENA_FORCE_INLINE bool _arena_is_pow2(size_t value)
{
    return (value && !(value & (value - 1)));
}

_ARENA_FORCE_INLINE bool _arena_is_aligned_to(arena_size_t value, size_t alignment)
{
    return (alignment > 0) && ((value % alignment) == 0);
}

_ARENA_FORCE_INLINE arena_size_t _arena_align_up(arena_size_t value, size_t alignment)
{
    size_t a = alignment ? alignment : 1;
    return (_arena_sadd(value, a - 1, ARENA_U64_MAX)) & ~(a - 1);
}

_ARENA_FORCE_INLINE size_t _arena_downcast_size(arena_size_t value, bool *overflow)
{
    if (overflow) *overflow = false;

    #if ARENA_SIZE_MAX == ARENA_U32_MAX
    // 32-bit system check
    if (value > ARENA_SIZE_MAX) {
        if (overflow) *overflow = true;
        return ARENA_SIZE_MAX;
    }
    #endif

    return (size_t)value;
}

_ARENA_FORCE_INLINE size_t _arena_get_platform_page_size()
{
    #if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
    long ps = sysconf(_SC_PAGESIZE);
    return ps > 0 ? (size_t)ps : ARENA_PAGE_DEFAULT_SIZE;
    #else
    return ARENA_PAGE_DEFAULT_SIZE;
    #endif
}

_ARENA_FORCE_INLINE void _arena_set_error(Arena *arena, ArenaError flag)
{
    if (NULL != arena) {
        arena->error = flag;
    }
}

_ARENA_FORCE_INLINE size_t _arena_calc_chunk_real_size(arena_size_t capacity)
{
    return _arena_downcast_size(_arena_sadd(sizeof(ArenaChunk), capacity, ARENA_SIZE_MAX), NULL);
}

_ARENA_FORCE_INLINE arena_size_t _arena_calc_chunk_capacity(size_t chunk_real_size)
{
    return (arena_size_t)(chunk_real_size - sizeof(ArenaChunk));
}

static inline void *_arena_alloc_chunk(size_t chunk_capacity, uint32_t alloc_type)
{
    /*
        Invariants:
        - chunk.capacity <= chunk_real_size - sizeof(ArenaChunk)
        - chunk.offset == 0
        - chunk.next   == NULL
    */
    size_t page_size = _arena_get_platform_page_size();

    ArenaChunk *chunk      = NULL;
    size_t chunk_real_size = _arena_calc_chunk_real_size(chunk_capacity);

    // to make actual assertion that actual chunk capacity >= chunk_capacity
    if (_arena_calc_chunk_capacity(chunk_real_size) < chunk_capacity) {
        ARENA_LOG("Critical error while allocating new chunk: allocation capacity is less than required capacity.");
        return NULL;
    }

    if (alloc_type == ARENA_ALLOC_TYPE_BIG) {
    #if ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
        chunk = mmap(
            NULL, 
            chunk_real_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1, 0
        );
        if (chunk == MAP_FAILED) goto exit_error;
        ARENA_LOG("New chunk allocated with `nmap` as %d bytes sized pages. Platform: %s Size: %zu", page_size, arena_platform_str(), chunk_real_size);
    #elif (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32)
        chunk = VirtualAlloc(NULL, chunk_real_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!chunk) goto exit_error;
        ARENA_LOG("New chunk allocated with with `VirtualAlloc` as %d bytes sized pages. Platform: %s Size: %zu", page_size, arena_platform_str(), chunk_real_size);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_LIBC
        chunk = malloc(chunk_real_size);
        if (!chunk) goto exit_error;
        ARENA_LOG("New chunk allocated with `malloc` as %d bytes sized pages. Platform: %s Size: %zu", page_size, arena_platform_str(), chunk_real_size);
    #else
        #error("Failed to apply platform dependent allocation. You are not supposed to see this error.")
    #endif
    } else {
        chunk = malloc(chunk_real_size); // small heap allocation
        if (!chunk) goto exit_error;
        ARENA_LOG("New chunk allocated with size of %d bytes. Platform: %s", chunk_real_size, arena_platform_str());
    }

    chunk->next     = NULL;
    chunk->offset   = 0;
    chunk->capacity = _arena_calc_chunk_capacity(chunk_real_size);

    ARENA_LOG("Chunk: base:%p capacity:%d", chunk->base, chunk->capacity);

    return chunk;

exit_error:
    ARENA_LOG("Failed to allocate arena chunk.");
    return NULL;
}

static inline size_t _arena_calc_realloc_size(const Arena *arena, arena_size_t required_chunk_capacity)
{
    arena_size_t cur_capacity = arena->last_chunk->capacity; // starting from the current capacity

    while (cur_capacity < required_chunk_capacity && cur_capacity < arena->max_capacity) {
        // there will be no overflow and no inf loop
        // because _arena_smul will return mul result or max capacity and loop will break
        cur_capacity = _arena_smul(cur_capacity, arena->growth_factor, arena->max_capacity);
    }

    bool overflow = false;
    size_t realloc_size = _arena_downcast_size(cur_capacity, &overflow);
    if ((arena->flags & ARENA_FLAG_DEBUG) && overflow == true) {
        ARENA_LOG("Warning: Overflow catched in `%s` while downcasting. Max system integer size will be returned.", __func__);
    }

    return _arena_calc_chunk_real_size(realloc_size); // returns real alloc size (chunk metadata + capacity)
}

static inline ArenaChunk *_arena_realloc(Arena* arena, size_t required_chunk_capacity)
{
    ArenaChunk *new_chunk = NULL;
    ArenaChunk *old_chunk = arena->last_chunk;

    size_t realloc_size = _arena_calc_realloc_size(arena, required_chunk_capacity); // real size of new chunk to realloc
    size_t free_size    = _arena_calc_chunk_real_size(arena->last_chunk->capacity); // real size of old chunk to free
    size_t memcpy_size  = free_size; // real size of copiable memory
    
    if (arena->alloc_type == ARENA_ALLOC_TYPE_BIG) {
    #if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32)
        new_chunk = (ArenaChunk*)VirtualAlloc(NULL, realloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!new_chunk) return NULL;
        arena_memcpy(new_chunk, old_chunk, memcpy_size);
        VirtualFree(old_chunk, free_size, MEM_RELEASE);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
        new_chunk = (ArenaChunk*)mmap(NULL, realloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_chunk == MAP_FAILED) return NULL;
        arena_memcpy(new_chunk, old_chunk, memcpy_size);
        munmap(old_chunk, free_size);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_LIBC
        new_chunk = (ArenaChunk*)realloc(old_chunk, realloc_size);
        if (!new_chunk) return NULL;
    #endif
    } else {
        new_chunk = (ArenaChunk*)realloc(old_chunk, realloc_size);
        if (!new_chunk) return NULL;
    }

    new_chunk->capacity = realloc_size - sizeof(ArenaChunk);
    arena->reserved     = new_chunk->capacity; 
    arena->head_chunk   = new_chunk;
    arena->last_chunk   = new_chunk;
    // because for realloc contract we store capacity of only one chunk
    // so `reserved` should be same as new chunk capacity (as we have only one chunk)

    return new_chunk;
}

static inline Arena arena_create_ex(ArenaConfig config)
{
    if (config.capacity == ARENA_CAPACITY_CHOOSE_FOR_ME_PLS) config.capacity = ARENA_CAPACITY_DEFAULT;
    config.max_capacity = config.max_capacity ? config.max_capacity : config.capacity;

    // resolve contract
    switch (config.growth_contract) {
        case ARENA_GROWTH_CONTRACT_CHUNKY: {
            if (config.growth_factor < ARENA_GROWTH_FACTOR_CHUNKY_MIN) config.growth_factor = ARENA_GROWTH_FACTOR_CHUNKY_MIN;
            if (config.growth_factor > ARENA_GROWTH_FACTOR_CHUNKY_MAX) config.growth_factor = ARENA_GROWTH_FACTOR_CHUNKY_MAX;
        } break;

        case ARENA_GROWTH_CONTRACT_REALLOC: {
            if (config.growth_factor > ARENA_GROWTH_FACTOR_REALLOC_8X) config.growth_factor = ARENA_GROWTH_FACTOR_REALLOC_8X;
            if (config.growth_factor < 2) config.growth_factor = ARENA_GROWTH_FACTOR_REALLOC_2X;
        } break;

        default: {
            config.growth_factor = ARENA_GROWTH_FACTOR_NONE;
        } break;
    }

    // 32-bit sys check
    bool overflow = false;
    size_t alloc_size = _arena_downcast_size(config.capacity, &overflow); // this is very important
    if (overflow) {
        // cannot allocate >4GB
        config.max_capacity = ARENA_CAPACITY_4GB - 1;
    }
    
    // finnaly allocate memory for arena
    uint32_t alloc_type = (alloc_size > ARENA_PAGE_ALIGN_THRESHOLD) ? ARENA_ALLOC_TYPE_BIG : ARENA_ALLOC_TYPE_SMALL;

    ArenaChunk *chunk = _arena_alloc_chunk(alloc_size, alloc_type);
    if (!chunk) return ARENA_EMPTY;

    if (config.flags & ARENA_FLAG_FILLZEROES) arena_memset(chunk->base, 0, chunk->capacity);
    
    return (Arena){
        .reserved        = chunk->capacity, // how much memory reserved in total
        .growth_contract = config.growth_contract,
        .growth_factor   = config.growth_factor,
        .max_capacity    = config.max_capacity,
        .flags           = config.flags,
        .debug           = (ArenaDebugInfo){0},
        .alloc_type      = alloc_type,
        .error           = ARENA_ERROR_NONE,
        .epoch           = 0,
        .head_chunk      = chunk,
        .last_chunk      = chunk
    };
}

static inline ArenaConfig arena_config_create(arena_size_t capacity, arena_size_t max_capacity, ArenaGrowthContract contract, size_t growth_factor, ArenaFlag flags)
{
    return (ArenaConfig){
        .capacity        = capacity,
        .flags           = flags,
        .growth_contract = contract,
        .growth_factor   = growth_factor,
        .max_capacity    = max_capacity
    };
}

static inline Arena arena_create(arena_size_t capacity)
{
    return arena_create_ex((ArenaConfig){
        .capacity        = capacity,
        .flags           = ARENA_FLAG_NONE,
        .growth_contract = ARENA_GROWTH_CONTRACT_FIXED,
        .growth_factor   = ARENA_GROWTH_FACTOR_NONE,
        .max_capacity    = ARENA_CAPACITY_256MB // i like this number
    });
}

static inline void arena_destroy(Arena *arena)
{
    if (!arena || !arena->head_chunk) return;

    ArenaChunk *chunk = arena->head_chunk;
    ArenaChunk *next_chunk = NULL;
    if (arena->alloc_type == ARENA_ALLOC_TYPE_BIG) {
        while (chunk != NULL) {
            next_chunk = chunk->next;
            ARENA_LOG("Chunk memory released at: %p", chunk);
    #if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32)
            VirtualFree(chunk, 0, MEM_RELEASE);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
            munmap(chunk, _arena_calc_chunk_real_size(chunk->capacity));
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_LIBC
            free(chunk);
    #endif
            chunk = next_chunk;
        }
    } else {
        while (chunk != NULL) {
            next_chunk = chunk->next;
            ARENA_LOG("Chunk memory released at: %p", chunk);
            free(chunk);
            chunk = next_chunk;
        }
    }

    arena->last_chunk              = NULL;
    arena->head_chunk              = NULL;
    arena->reserved                = 0;
    arena->flags                   = 0;
    arena->growth_contract         = 0;
    arena->growth_factor           = 0;
    arena->max_capacity            = 0;
    arena->alloc_type              = 0;
    arena->error                   = ARENA_ERROR_NONE;
    arena->debug.end               = NULL;
    arena->debug.bytes_lost        = 0;
    arena->debug.total_allocations = 0;

    ARENA_LOG("Arena destroyed. Platform: %s", arena_platform_str());
}

static inline ArenaMemory arena_alloc(Arena *arena, arena_size_t size, size_t alignment)
{
    void *p = arena_alloc_raw(arena, size, alignment);
    if (!p) return (ArenaMemory){NULL, NULL,  0, 0, 0};
    return (ArenaMemory){
        .chunk     = arena->last_chunk,
        .data      = p,
        .size      = size,
        .offset    = (arena_ptr_t)p - (arena_ptr_t)arena->last_chunk->base,
        .alignment = alignment,
        .epoch     = arena->epoch
    };
}

static inline void 
_arena_calc_alloc_data(
    ArenaChunk *last_chunk, arena_size_t size, 
    size_t alignment, arena_ptr_t *address, 
    arena_ptr_t *aligned_address, 
    arena_size_t *new_offset, arena_size_t *lost_bytes)
{
    *address = (arena_ptr_t)last_chunk->base + last_chunk->offset;
    *aligned_address = (arena_ptr_t)_arena_align_up((*address), alignment);
    *lost_bytes = (arena_size_t)((*aligned_address) - (*address));
    *new_offset = ((*aligned_address) - (arena_ptr_t)last_chunk->base) + size;
}

static inline void *arena_alloc_raw(Arena *arena, arena_size_t size, size_t alignment)
{
    if (!arena || size == 0) return NULL;
    if (!_arena_is_pow2(alignment)) {
        _arena_set_error(arena, ARENA_ERROR_INVALID_ALIGNMENT);
        return NULL;
    }
    // if alignment is forced then every address should be aligned to CPU cache line size
    if (arena->flags & ARENA_FLAG_ENFORCE_ALIGNMENT)
        alignment = ARENA_ALIGN_CACHELINE;

    arena_ptr_t  addr         = 0;
    arena_ptr_t  aligned_addr = 0;
    arena_size_t lost_bytes   = 0;
    arena_size_t new_offset   = 0;
        
    _arena_calc_alloc_data(arena->last_chunk, size, alignment, &addr, &aligned_addr, &new_offset, &lost_bytes);
    
    if (new_offset > arena->last_chunk->capacity) {
        bool has_grown = arena_grow(arena, new_offset);
        if (!has_grown) {
            _arena_set_error(arena, ARENA_ERROR_OOM);
            ARENA_LOG(
                "Failed to allocate `%d` bytes on arena (OOM error. Requested: "ARENA_SIZE_FMT", Reserved: "ARENA_SIZE_FMT")",
                size + lost_bytes, size, arena->reserved
            );
            return NULL;
        }
        // important recalc after growth !!!
        _arena_calc_alloc_data(arena->last_chunk, size, alignment, &addr, &aligned_addr, &new_offset, &lost_bytes); 
    }
    
    arena->last_chunk->offset = new_offset;
    arena_size_t alloc_size   = size + lost_bytes;
    
    ARENA_LOG(
        "Allocated `%d` bytes on arena (align = %zu, loss = %d, requested = %d)",
        alloc_size, alignment, lost_bytes, size
    );
    
    if (arena->flags & ARENA_FLAG_DEBUG) {
        arena->debug.end        = (void*)((arena_ptr_t)arena->last_chunk->base + new_offset);
        arena->debug.bytes_lost += lost_bytes;
        arena->debug.total_allocations++;
    }

    _arena_set_error(arena, ARENA_ERROR_NONE);
    return (void*)aligned_addr;
}

static inline ArenaMemory arena_alloc_zero(Arena *arena, arena_size_t size, size_t alignment)
{
    ARENA_LOG("Arena `arena_alloc_zero` called.");

    void *p = arena_alloc_raw(arena, size, alignment);
    if (!p) return (ArenaMemory){NULL, NULL, 0, 0, 0};

    arena_memset(p, 0, size);

    return (ArenaMemory){
        .chunk     = arena->last_chunk,
        .data      = p,
        .size      = size,
        .offset    = (arena_ptr_t)p - (arena_ptr_t)arena->last_chunk->base,
        .alignment = alignment
    };
}

static inline bool arena_grow(Arena *arena, arena_size_t required_capacity)
{
    if (!arena || required_capacity <= arena->last_chunk->capacity || arena->growth_contract == ARENA_GROWTH_CONTRACT_FIXED) return false;
    if (required_capacity > arena->max_capacity) {
        _arena_set_error(arena, ARENA_ERROR_OOM);
        return false;
    }
    
    switch (arena->growth_contract) {
        case ARENA_GROWTH_CONTRACT_FIXED: return false;
        
        case ARENA_GROWTH_CONTRACT_REALLOC: {
            ArenaChunk *new_chunk = _arena_realloc(arena, required_capacity);
            if (!new_chunk) return false;
            
            arena->last_chunk = new_chunk;

            ARENA_LOG(
                "Arena reallocated at: %p\n"
                "    Capacity:          "ARENA_SIZE_FMT"\n"
                "    Growth factor:     "ARENA_SIZE_FMT"X\n"
                "    Required capacity: "ARENA_SIZE_FMT"\n"
                "    Offset:            "ARENA_SIZE_FMT"\n"
                "    Base:              %p\n",
                arena->last_chunk,
                arena->last_chunk->capacity,
                arena->growth_factor,
                required_capacity,
                arena->last_chunk->offset,
                arena->last_chunk->base
            );
        } break;

        case ARENA_GROWTH_CONTRACT_CHUNKY: {
            arena_size_t chunk_capacity = arena->growth_factor;
            if (chunk_capacity < required_capacity)
                // add extra bytes to prevent alloc failure due to address alignment
                chunk_capacity = _arena_align_up(required_capacity, ARENA_ALIGN_512B);

            ARENA_LOG("New chunk capacity: "ARENA_SIZE_FMT, chunk_capacity);

            if (arena->reserved + chunk_capacity > arena->max_capacity) {
                ARENA_LOG("Failed to allocate new chunk. Max capacity overflow.");
                _arena_set_error(arena, ARENA_ERROR_OOM);
                return false;
            }

            ArenaChunk *chunk = _arena_alloc_chunk(_arena_downcast_size(chunk_capacity, NULL), arena->alloc_type);
            if (!chunk) return false;

            arena->last_chunk->next = chunk;
            arena->last_chunk = chunk;
            arena->reserved += chunk->capacity;

            ARENA_LOG(
                "New chunk added at: %p\n"
                "    Capacity:          "ARENA_SIZE_FMT"\n"
                "    Growth factor:     "ARENA_SIZE_FMT"\n"
                "    Required capacity: "ARENA_SIZE_FMT"\n"
                "    Offset:            "ARENA_SIZE_FMT"\n"
                "    Base:              %p\n",
                chunk,
                chunk->capacity,
                arena->growth_factor,
                required_capacity,
                chunk->offset,
                chunk->base
            );
        } break;

        default: return false;
    }

    if (arena->flags & ARENA_FLAG_RESET_AFTER_GROW) arena_reset(arena); // Bye bye =D
    
    return true;
}

static inline void *arena_memory_resolve(const Arena *arena, ArenaMemory *memory)
{
    if (!arena || !memory || !arena->head_chunk || memory->epoch != arena->epoch) return NULL;    

    if (arena->growth_contract == ARENA_GROWTH_CONTRACT_REALLOC) {
        memory->chunk = arena->head_chunk;
    }

    memory->data = (void*)((arena_ptr_t)memory->chunk->base + memory->offset);
    
    return memory->data;
}

static inline bool arena_reset(Arena *arena)
{
    /*
    - Invalidates all previous pointers
    - Does not free the memory
    - Does not change chunks structure
    - Literally just resets arena (but with all previously allocated memory)
    - O(1) =D
    */
    if (!arena || !arena->last_chunk) goto reset_failure;

    if (arena->growth_contract == ARENA_GROWTH_CONTRACT_CHUNKY) {
        for (ArenaChunk *c = arena->head_chunk; NULL != c; c = c->next ) {
            _ARENA_PREFETCH(c->next);
            ARENA_LOG("Chunk resetted at: %p", c);
            c->offset = 0;
        }
        arena->epoch++;
        goto reset_success;
    } else {
        arena->last_chunk->offset = 0;
        arena->epoch++;
        goto reset_success;
    }

reset_failure:
    ARENA_LOG("Arena reset failed");
    return false;
reset_success:
    ARENA_LOG("Arena reset success");
    return true;
}

#ifndef ARENA_USE_STD_STRING
static inline void *arena_memcpy(void *dst, const void *src, size_t count)
{
    void *start = dst;
    while (count >= 8) { // type cast ol trick
        *(uint64_t*)start = *(uint64_t*)src;
        start += 8; src += 8;
        count -= 8;
    }

    while (count--) *(uint8_t*)start++ = *(uint8_t*)src++;
    
    return dst;
}

static inline void *arena_memset(void *dst, int value, size_t size)
{
    // who said that fast algorithms should look readable?
    if (!dst || size == 0) return NULL;

    int word = 0;
    uint64_t pattern64 = (uint8_t)value;
    
    if (size >= 0x4000) {
        word = 8;
        pattern64 |= (pattern64 << 8);
        pattern64 |= (pattern64 << 16);
        pattern64 |= (pattern64 << 32);
    } else if (size >= 0x1000) {
        word = 4;
        pattern64 |= (pattern64 << 8);
        pattern64 |= (pattern64 << 16);
    }

    uint8_t *p8 = (uint8_t*)dst;
    
    if (word) {
        size_t offset = (uintptr_t)dst & (word-1);
        
        if (offset) {
            size_t align = word - offset;
            if (size < align) align = size;
            for (int i = 0; i < align; ++i) *p8++ = (uint8_t)value;
            size -= align;
        }

        if (word == 4) {
            uint32_t *p32 = (uint32_t*)p8;
            size_t blocks  = size / word;
            uint32_t pattern32 = (uint32_t)pattern64;

            while (blocks--) {
                *p32++ = pattern32;
            }

            p8 = (uint8_t*)p32;
            size = size % word;
        } else if (word == 8) {
             uint64_t *p64 = (uint64_t*)p8;
             size_t blocks = size / word;

            while (blocks--) {
                *p64++ = pattern64;
            }

            p8 = (uint8_t*)p64;
            size = size % word;
        }

        while (size--) {
            *p8++ = (uint8_t)value;
        }
    } else {
        while (size--) {
            *p8++ = (uint8_t)value;
        }
    }

    return dst;
}

static inline char *arena_strdup(Arena *arena, const char *src)
{
    if (!arena || arena->reserved == 0 || !src) return NULL;
    void *dst = arena_alloc_raw(arena, arena_strlen(src)+1, alignof(char));
    if (dst) arena_memcpy(dst, src, arena_strlen(src)+1);
    return dst;
}

static inline size_t arena_strlen(const char *str) // USA international debt growth simulator
{
    size_t usa_international_debt = 0;
    while (*str++) {
        usa_international_debt++;
    }
    return usa_international_debt;
}

#define _ARENA_HAS_ZERO_BYTE(v) (((v) - 0x0101010101010101ull) & ~(v) & 0x8080808080808080ull)
// 1 nanosec faster on O0, O1 and O2 than arena_strlen (still slower than libc strlen)
static inline size_t arena_strlen_fast(const char *str)
{
    const char *p = str;
    while ((arena_ptr_t)p & 7) {
        if (*p == 0) return p - str;
        p++;
    }

    const uint64_t *word = (uint64_t*)p;
    while (1) {
        uint64_t value = *word;
        if (_ARENA_HAS_ZERO_BYTE(value)) break;
        word++;
    }

    const char *byte = (const char*)word;
    for (char i = 0; i < 8; ++i) {
        if (byte[i] == 0) return (byte + i) - str;
    }

    return 0; // should be unreachable actually
}
#endif

_ARENA_FORCE_INLINE long long arena_abs(long long value)
{
    long long result;
    const int mask = value >> sizeof(value) * CHAR_BIT - 1;
    result = (value ^ mask) - mask;
    return result;
}

_ARENA_FORCE_INLINE ArenaMark arena_mark(const Arena *arena)
{
    return (ArenaMark){ 
        .offset = arena->last_chunk->offset,
        .epoch  = arena->epoch,
        .chunk  = arena->last_chunk
    };
}

static inline bool arena_restore(Arena *arena, ArenaMark mark, bool poison_memory)
{
    if (!arena || !arena->last_chunk || arena->epoch != mark.epoch) return false;

    if (poison_memory) {
        arena_ptr_t address = (arena_ptr_t)mark.chunk->base + mark.offset;
        arena_size_t size   = mark.chunk->offset - mark.offset;
        arena_memset((void*)address, _ARENA_POISON_RESET, size);
    }

    for (ArenaChunk *c = mark.chunk; c != NULL; c = c->next) {
        _ARENA_PREFETCH(c->next);
        c->offset = 0;
    }

    arena->last_chunk = mark.chunk;
    return true;
}

/* Helper macros */
#define arena_alloc_struct(pArena, type)           ((type*)arena_alloc_raw((pArena), sizeof(type), alignof(type)))
#define arena_alloc_array(pArena, size, type)      ((size) == 0 ? NULL : (type*)arena_alloc_raw((pArena), sizeof(type)*size, alignof(type)))
#define arena_alloc_struct_zero(pArena, type)      ((type*)arena_alloc_zero((pArena), sizeof(type), alignof(type)))
#define arena_alloc_array_zero(pArena, size, type) ((size) == 0 ? NULL : (type*)arena_alloc_zero((pArena), sizeof(type)*size, alignof(type)))

#endif // ARENA_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // ARENA_H_
