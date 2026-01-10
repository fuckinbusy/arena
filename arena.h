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

#include <stdbool.h>
#include <stdlib.h>
#include <stdalign.h>

// #include <stdio.h>

#include <stdint.h>
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

#if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32) || (ARENA_PLATFORM == _ARENA_PLATFORM_WIN64)
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
    error("Undefined platform")
#endif

#ifndef ARENA_NOMINAX
    #define ARENA_MAX(a, b) ((a) - (((a) - (b)) & (((a) - (b)) >> 31))) // damn
    #define ARENA_MIN(a, b) ((b) + (((a) - (b)) & (((a) - (b)) >> 31)))
#endif

#define ARENA_U64_MAX  0xffffffffffffffffull
#define ARENA_U32_MAX  0xffffffffu
#define ARENA_SIZE_MAX SIZE_MAX
#define ARENA_PTR_MAX  MAXINT_PTR

#define _ARENA_POISON_ALLOC         0xCD   // arena memory poisoning value after allocating memory from arena
#define _ARENA_POISON_RESET         0xDD   // arena memory poisoning value after resetting arena
#define ARENA_PAGE_ALIGN_THRESHOLD 0x2000  // used to identify when to switch to platform specific allocation 
#define ARENA_PAGE_DEFAULT_SIZE    0x1000  // for libc universal platform 

typedef enum ArenaGrowthContract {
    ARENA_GROWTH_CONTRACT_FIXED       = 0,
    ARENA_GROWTH_CONTRACT_EXPONENTIAL = 1,
    ARENA_GROWTH_CONTRACT_LINEAR      = 2,
    ARENA_GROWTH_CONTRACT_PAGE        = 3,
    ARENA_GROWTH_CONTRACT_DEFAULT     = ARENA_GROWTH_CONTRACT_FIXED,
} ArenaGrowthContract;

typedef enum ArenaGrowthFactor {
    ARENA_GROWTH_FACTOR_NONE        = 0x0,    // none =D
    ARENA_GROWTH_FACTOR_EXPONENTIAL = 0x2,    // for mul operation
    ARENA_GROWTH_FACTOR_LINEAR      = 0x1000,  // for add operation
    ARENA_GROWTH_FACTOR_PAGE        = 0x1000, // reflects size of page (not user's platform actual page size)
    ARENA_GROWTH_FACTOR_DEFAULT     = ARENA_GROWTH_FACTOR_NONE,
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

typedef enum ArenaAlignment {
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
    ARENA_ALIGN_CACHELINE = 64, // TODO make sure this is used for addresses only and not for the size of allocations!!!
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
    ArenaAlignment      base_alignment;
    ArenaGrowthContract growth_contract;
    size_t              growth_factor;
    ArenaFlag           flags;
} ArenaConfig;

typedef struct ArenaDebugInfo {
    uint8_t      *end;              // address to the end of the last allocated block of memory
    arena_size_t epoch;       // current state of the arena
    arena_size_t bytes_lost;        // bytes loss due to alignment
    size_t       total_allocations; // obviously number of total allocations
} ArenaDebugInfo;

typedef struct ArenaMemory {
    void         *data;     // pointer to the memory (invalidates after arena_grow())
    arena_size_t size;      // memory size
    arena_size_t offset;    // offset from the arena base to the memory base/data
    size_t       alignment; // how data is aligned in this memory
} ArenaMemory;

typedef struct Arena {
    uint8_t             *base;           // arena base address
    arena_size_t        offset;          // represents and offset to the next available memory region, also reflects amount of memory taken by user
    arena_size_t        capacity;        // size of memory that is available for user in this arena
    arena_size_t        max_capacity;    // max arena capacity (machine dependent), it shows how big this arena can be if it can grow
    ArenaAlignment      base_alignment;  // arena base alignment (8b or 16b is good, 128b and 256b for SIMD)
    arena_size_t        growth_factor;   // how fast grows (depends on contract)
    ArenaGrowthContract growth_contract; // arena growth contract (how to grow)
    ArenaFlag           flags;           // arena flags - ARENA_FLAG_...
    ArenaError          error;           // error flag
    uint32_t            alloc_type;      // reflects how arena memory was originally allocated and must not change during arena lifetime

    ArenaDebugInfo      debug;
} Arena;

#define ARENA_EMPTY ((Arena){0})

static inline Arena arena_create_ex(ArenaConfig config);
static inline ArenaConfig arena_config_create(arena_size_t capacity, arena_size_t max_capacity, ArenaAlignment base_alignment, ArenaGrowthContract contract, size_t growth_factor, ArenaFlag flags);
static inline Arena arena_create(size_t capacity);
static inline void arena_destroy(Arena *arena);
static inline ArenaMemory arena_alloc(Arena *arena, arena_size_t size, size_t alignment);
static inline void *arena_alloc_raw(Arena *arena, arena_size_t size, size_t alignment);
static inline ArenaMemory arena_alloc_zero(Arena *arena, arena_size_t size, size_t alignment);
static inline void *arena_alloc_zero_raw(Arena *arena, arena_size_t size, size_t alignment);
static inline void arena_reset(Arena *arena);
static inline bool arena_grow(Arena *arena, arena_size_t grow_size);
static inline bool arena_memory_resolve(Arena *arena, ArenaMemory *memory);

static inline bool arena_memcpy_within(Arena *arena, void *dst, const void *src, size_t count);
static inline void *arena_memset_within(Arena *arena, void *dst, int value, size_t count);
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

_ARENA_FORCE_INLINE arena_size_t _arena_smul(arena_size_t a, arena_size_t b, arena_size_t max)
{
    return (a > (max / b)) ? max : (a * b);
}

_ARENA_FORCE_INLINE bool _arena_is_pow2(size_t value)
{
    return ((value > 0) && ((value & (value - 1)) == 0));
}

_ARENA_FORCE_INLINE bool _arena_is_aligned_to(arena_size_t value, size_t alignment)
{
    return (alignment > 0) && ((value % alignment) == 0);
}

_ARENA_FORCE_INLINE arena_size_t _arena_align_up(arena_size_t value, size_t alignment)
{
    if (alignment == 0) return value; // prevents from very dangerous UB
    return (_arena_sadd(value, alignment - 1, ARENA_U64_MAX)) & ~(alignment - 1);
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
    #if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32) || (ARENA_PLATFORM == _ARENA_PLATFORM_WIN64)

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

static inline void *_arena_alloc_arena(size_t alloc_size, uint32_t alloc_type)
{
    void *mem = NULL;
    size_t page_size = _arena_get_platform_page_size();

    if (alloc_type == ARENA_ALLOC_TYPE_BIG) {
    #if ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
        alloc_size = _arena_align_up(alloc_size, page_size);
        mem = mmap(
            NULL, 
            alloc_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1, 0
        );
        if (mem == MAP_FAILED) mem = NULL;
        ARENA_LOG("Arena created. Memory allocated with `nmap` as %d bytes sized pages. Platform: %s Size: %zu", page_size, arena_platform_str(), alloc_size);
    #elif (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32) || (ARENA_PLATFORM == _ARENA_PLATFORM_WIN64)
        mem = VirtualAlloc(NULL, alloc_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        ARENA_LOG("Arena created. Memory allocated with `VirtualAlloc` as %d bytes sized pages. Platform: %s Size: %zu", page_size, arena_platform_str(), alloc_size);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_LIBC
        alloc_size = _arena_align_up(alloc_size, page_size);
        mem = malloc(alloc_size);
        ARENA_LOG("Arena created. Memory allocated with `malloc` as %d bytes sized pages. Platform: %s Size: %zu", page_size, arena_platform_str(), alloc_size);
    #else
        error("Failed to apply platform dependent allocation. You are not supposed to see this error.")
    #endif
    } else {
        mem = malloc(alloc_size); // small heap allocation
        ARENA_LOG("Arena created. Memory allocated with size of %d bytes. Platform: %s Size: %zu", alloc_size, arena_platform_str(), alloc_size);
    }

    ARENA_LOG("Arena allocated: base:%p alloc_size:%d", mem, alloc_size);

    return mem;
}

static inline Arena arena_create_ex(ArenaConfig config)
{
    if (config.capacity == ARENA_CAPACITY_CHOOSE_FOR_ME_PLS) config.capacity = ARENA_CAPACITY_DEFAULT;
    if (config.base_alignment == 0) config.base_alignment = ARENA_ALIGN_DEFAULT;

    size_t page_size  = _arena_get_platform_page_size();

    // resolve contract
    switch (config.growth_contract) {
        case ARENA_GROWTH_CONTRACT_LINEAR: {
            config.growth_factor = config.growth_factor ? config.growth_factor : ARENA_GROWTH_FACTOR_LINEAR;
            config.max_capacity  = config.max_capacity ? config.max_capacity : ARENA_CAPACITY_MAX;
            if (config.growth_contract < ARENA_GROWTH_FACTOR_LINEAR) 
                ARENA_LOG("WARNING: Growth factor is too small for linear growth! Perfomance will decrease!");
        } break;

        case ARENA_GROWTH_CONTRACT_EXPONENTIAL: {
            config.growth_factor = config.growth_factor ? config.growth_factor : ARENA_GROWTH_FACTOR_EXPONENTIAL;
            config.max_capacity  = config.max_capacity ? config.max_capacity : ARENA_CAPACITY_MAX;
        } break;

        case ARENA_GROWTH_CONTRACT_PAGE: {
            config.growth_factor = page_size; // ignoring config value on purpose TODO
            config.max_capacity  = config.max_capacity ? config.max_capacity : ARENA_CAPACITY_MAX;
        } break;

        default: {
            config.growth_factor = ARENA_GROWTH_FACTOR_NONE;
            config.max_capacity  = config.max_capacity ? config.max_capacity : config.capacity;
        } break;
    }

    // 32-bit sys check
    bool overflow = false;
    size_t alloc_size = _arena_downcast_size(config.capacity, &overflow); // this is very important
    if (overflow) {
        // cannot allocate >4GB
        config.max_capacity = alloc_size;
    }
    
    // finnaly allocate memory for arena
    uint32_t alloc_type = 
        (alloc_size > ARENA_PAGE_ALIGN_THRESHOLD) ? ARENA_ALLOC_TYPE_BIG : ARENA_ALLOC_TYPE_SMALL;
    void *mem = _arena_alloc_arena(alloc_size, alloc_type);
    if (!mem) return ARENA_EMPTY;

    if (config.flags & ARENA_FLAG_FILLZEROES) arena_memset(mem, 0, alloc_size);
    
    return (Arena){
        .base            = (uint8_t*)mem,
        .capacity        = alloc_size,
        .offset          = 0,
        .base_alignment  = config.base_alignment,
        .growth_contract = config.growth_contract,
        .growth_factor   = config.growth_factor,
        .max_capacity    = config.max_capacity,
        .flags           = config.flags,
        .debug           = (ArenaDebugInfo){0},
        .alloc_type      = alloc_type,
        .error           = ARENA_ERROR_NONE,
    };
}

static inline ArenaConfig arena_config_create(arena_size_t capacity, arena_size_t max_capacity, ArenaAlignment base_alignment, ArenaGrowthContract contract, size_t growth_factor, ArenaFlag flags)
{
    return (ArenaConfig){
        .base_alignment  = base_alignment,
        .capacity        = capacity,
        .flags           = flags,
        .growth_contract = contract,
        .growth_factor   = growth_factor,
        .max_capacity    = max_capacity
    };
}

static inline Arena arena_create(size_t capacity)
{
    return arena_create_ex((ArenaConfig){
        .base_alignment  = ARENA_ALIGN_DEFAULT, 
        .capacity        = capacity,
        .flags           = ARENA_FLAG_DEBUG,
        .growth_contract = ARENA_GROWTH_CONTRACT_FIXED,
        .growth_factor   = ARENA_GROWTH_FACTOR_NONE,
        .max_capacity    = 0
    });
}

static inline void arena_destroy(Arena *arena)
{
    if (!arena || !arena->base) return;

    if (arena->alloc_type == ARENA_ALLOC_TYPE_BIG) {
    #if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32) || (ARENA_PLATFORM == _ARENA_PLATFORM_WIN64)
        VirtualFree(arena->base, 0, MEM_RELEASE);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
        munmap(arena->base, arena->capacity);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_LIBC
        free(arena->base);
    #endif
    } else {
        free(arena->base);
    }
    
    arena->base                    = NULL;
    arena->base_alignment          = 0;
    arena->capacity                = 0;
    arena->flags                   = 0;
    arena->growth_contract         = 0;
    arena->growth_factor           = 0;
    arena->max_capacity            = 0;
    arena->offset                  = 0;
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
    if (!p) return (ArenaMemory){NULL, 0, 0, 0};
    return (ArenaMemory){
        .data      = p,
        .size      = size,
        .offset    = (arena_ptr_t)p - (arena_ptr_t)arena->base,
        .alignment = alignment
    };
}

static inline void 
_arena_calc_alloc_data(
    Arena *arena, arena_size_t size, 
    size_t alignment, arena_ptr_t *address, 
    arena_ptr_t *aligned_address, 
    arena_size_t *new_offset, arena_size_t *lost_bytes)
{
    *address = (arena_ptr_t)arena->base + arena->offset;
    *aligned_address = (arena_ptr_t)_arena_align_up((*address), alignment);
    *lost_bytes = (arena_size_t)((*aligned_address) - (*address));
    *new_offset = ((*aligned_address) - (arena_ptr_t)arena->base) + size;
}

static inline void *arena_alloc_raw(Arena *arena, arena_size_t size, size_t alignment)
{
    if (!arena || size == 0) return NULL;

    if (size > arena->max_capacity) {
        _arena_set_error(arena, ARENA_ERROR_OOM);
        return NULL;
    }

    if (!_arena_is_pow2(alignment)) {
        _arena_set_error(arena, ARENA_ERROR_INVALID_ALIGNMENT);
        return NULL;
    }

    // if alignment is forced then every address should be aligned to `arena->base_alignment`
    if (arena->flags & ARENA_FLAG_ENFORCE_ALIGNMENT)
        alignment = arena->base_alignment;

    arena_ptr_t  addr         = 0;
    arena_ptr_t  aligned_addr = 0;
    arena_size_t lost_bytes   = 0;
    arena_size_t new_offset   = 0;

    _arena_calc_alloc_data(arena, size, alignment, &addr, &aligned_addr, &new_offset, &lost_bytes);

    if (new_offset > arena->capacity) {
        if (arena->growth_contract == ARENA_GROWTH_CONTRACT_FIXED) {
            _arena_set_error(arena, ARENA_ERROR_OOM);
            return NULL;
        }

        ARENA_LOG("New size for growth: "ARENA_SIZE_FMT"", (new_offset - arena->capacity) + arena->capacity);
        bool has_grown = arena_grow(arena, (new_offset - arena->capacity) + arena->capacity);
        
        if (has_grown) {
            // if arena was resetted then must recalc all alloc related data here
            _arena_calc_alloc_data(arena, size, alignment, &addr, &aligned_addr, &new_offset, &lost_bytes);
            _arena_set_error(arena, ARENA_ERROR_NONE);
        } else {
            _arena_set_error(arena, ARENA_ERROR_OOM);
            return NULL;
        }
    }

    arena_size_t alloc_size = new_offset - arena->offset;

    // ARENA_LOG(
    //     "arena_alloc--->\n"
    //     "\tAddress:           %p\n"
    //     "\tAddress aligned:   %p\n"
    //     "\tAlloc address:     %p + "ARENA_SIZE_FMT" (alignment) = %p\n"
    //     "\tIs base aligned:   %p %% %d -> %s\n"
    //     "\tOffset before:     "ARENA_SIZE_FMT" (base + offset before = %p) <- old current\n"
    //     "\tOffset after:      "ARENA_SIZE_FMT" (base + offset after  = %p) <- new current\n"
    //     "\tUsed:              "ARENA_SIZE_FMT" bytes\n"
    //     "\tLost to alignment: "ARENA_SIZE_FMT" bytes\n",
    //     (void*)addr,
    //     (void*)aligned_addr, 
    //     (void*)addr, lost_bytes, (void*)aligned_addr,
    //     aligned_addr, arena->base_alignment, _arena_is_aligned_to(aligned_addr, arena->base_alignment) ? "T" : "F",
    //     arena->offset, (void*)addr,
    //     new_offset, (void*)((arena_ptr_t)arena->base + new_offset),
    //     alloc_size,
    //     lost_bytes
    // );

    ARENA_LOG(
        "Commited `%d` bytes on arena (align = %zu, loss = %d, requested = %d)",
        (new_offset - arena->offset), alignment, lost_bytes, size
    );

    arena->offset = new_offset;
    
    if (arena->flags & ARENA_FLAG_DEBUG) {
        arena->debug.end        = (void*)((arena_ptr_t)arena->base + new_offset);
        arena->debug.bytes_lost += lost_bytes;
        arena->debug.total_allocations++;
    }

    return (void*)aligned_addr;
}

static inline ArenaMemory arena_alloc_zero(Arena *arena, arena_size_t size, size_t alignment)
{
    ARENA_LOG("Arena `arena_alloc_zero` called.");

    void *p = arena_alloc_raw(arena, size, alignment);
    if (!p) return (ArenaMemory){NULL, 0, 0, 0};

    arena_memset(p, 0, size);

    return (ArenaMemory){
        .data      = p,
        .size      = size,
        .offset    = (arena_ptr_t)p - (arena_ptr_t)arena->base,
        .alignment = alignment
    };
}

static inline size_t _arena_calc_new_alloc_size(Arena *arena, arena_size_t required_capacity)
{
    arena_size_t new_size = arena->capacity;

    switch (arena->growth_contract) {
        case ARENA_GROWTH_CONTRACT_LINEAR: {
            while (new_size < required_capacity && new_size < arena->max_capacity) {
                new_size = _arena_sadd(new_size, arena->growth_factor, arena->max_capacity);
            }
        } break;

        case ARENA_GROWTH_CONTRACT_EXPONENTIAL: {
            while (new_size < required_capacity && new_size < arena->max_capacity) {
                new_size = _arena_smul(new_size, arena->growth_factor, arena->max_capacity);
            }
        } break;

        case ARENA_GROWTH_CONTRACT_PAGE: {
            uint32_t pages_required = (uint32_t)((required_capacity + (arena->growth_factor - 1)) / arena->growth_factor);
            new_size = arena->capacity + (arena->growth_factor * pages_required);
        } break;

        default: return 0;
    }

    if (new_size < required_capacity) return 0;

    bool overflow = false;
    size_t alloc_size = _arena_downcast_size(new_size, &overflow);
    if (arena->flags & ARENA_FLAG_DEBUG && overflow == true) {
        ARENA_LOG("Warning: Overflow catched in `%s` while downcasting. Max system integer size will be returned.", __func__);
    }

    return alloc_size;
}

// Resizes arena memory storage
// Arena is reset to empty state after grow if ARENA_FLAG_RESET_AFTER_GROW is set
// All previously obtained pointers are invalidated after grow
// Use arena_memory_resolve to validate back old pointers if needed
static inline bool arena_grow(Arena *arena, arena_size_t required_size)
{
    if (!arena || arena->capacity == 0 || arena->growth_contract == ARENA_GROWTH_CONTRACT_FIXED) return false;
    if (arena->capacity >= arena->max_capacity || arena->capacity > required_size || required_size > arena->max_capacity) return false;
    
    size_t alloc_size = _arena_calc_new_alloc_size(arena, required_size);
    if (alloc_size == 0 || alloc_size <= arena->capacity || alloc_size > arena->max_capacity) return false; // explicit assert for growing

    void *new_base = NULL;
    void *old_base = arena->base;
    
    if (arena->alloc_type == ARENA_ALLOC_TYPE_BIG) {
    #if (ARENA_PLATFORM == _ARENA_PLATFORM_WIN32) || (ARENA_PLATFORM == _ARENA_PLATFORM_WIN64)
        new_base = VirtualAlloc(NULL, alloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!new_base) return false;
        arena_memcpy(new_base, old_base, arena->capacity);
        VirtualFree(old_base, arena->capacity, MEM_RELEASE);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_UNIX
        new_base = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_base == MAP_FAILED) return false;
        arena_memcpy(new_base, old_base, arena->capacity);
        munmap(old_base, arena->capacity);
    #elif ARENA_PLATFORM == _ARENA_PLATFORM_LIBC
        new_base = realloc(old_base, alloc_size);
        if (!new_base) return false;
        // arena_memcpy(new_base, old_base, arena->capacity);
    #endif
    } else {
        new_base = realloc(old_base, alloc_size);
        if (!new_base) return false;
        // arena_memcpy(new_base, old_base, arena->capacity);
    }
    
    ARENA_LOG("Arena grows from %zu to %zu (requested size: "ARENA_SIZE_FMT")", arena->capacity, alloc_size, required_size);
    arena->base     = new_base;
    arena->capacity = alloc_size;

    if (arena->flags & ARENA_FLAG_RESET_AFTER_GROW) arena_reset(arena); // Bye bye =D
    
    return true;
}

static inline bool arena_memory_resolve(Arena *arena, ArenaMemory *memory)
{
    if (
        !arena                             || 
        !memory                            || 
        (arena->capacity < memory->offset) || 
        arena->capacity == 0               || 
        arena->offset < memory->offset
    ) return false;
    
    memory->data = (void*)((arena_ptr_t)arena->base + memory->offset);
    
    return true;
}

static inline void arena_reset(Arena *arena)
{
    if (arena && arena->base && arena->capacity > 0) {
        arena->offset = 0;
        // memset(arena->base, _ARENA_POISON_RESET, arena->capacity);
        ARENA_LOG("Arena reset");
        if (arena->flags & ARENA_FLAG_DEBUG) arena->debug.epoch++;
        _arena_set_error(arena, ARENA_ERROR_NONE);
    }
}

#ifndef ARENA_USE_STD_STRING
static inline void *arena_memcpy(void *dst, const void *src, size_t count)
{
    while (count >= 8) { // type cast ol trick
        *(uint64_t*)dst = *(uint64_t*)src;
        dst += 8; src += 8;
        count -= 8;
    }

    while (count--) *(uint8_t*)dst++ = *(uint8_t*)src++;
    
    return dst;
}

static inline void *arena_memset(void *dst, int value, size_t size)
{
    // who said that fast algorithms should look readable?
    if (!dst || size == 0) return NULL;

    int word = 0;
    uint64_t pattern64 = (uint8_t)value;
    
    if (size >= 0x10000) {
        word = 8;
        pattern64 |= (pattern64 << 8);
        pattern64 |= (pattern64 << 16);
        pattern64 |= (pattern64 << 32);
    } else if (size >= 0x4000) {
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
    if (!arena || arena->capacity == 0 || !src) return NULL;
    void *dst = arena_alloc_raw(arena, arena_strlen(src)+1, alignof(char));
    if (dst) arena_memcpy(dst, src, arena_strlen(src)+1);
    return dst;
}

static inline size_t arena_strlen(const char *str) // USA international debt growth simulator
{
    size_t usa_international_debt = 0;
    while (str++) {
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

static inline bool arena_memcpy_within(Arena *arena, void *dst, const void *src, size_t count)
{
    if (!dst || !src || count == 0) return false;

    arena_ptr_t dst_start   = (arena_ptr_t)dst;
    arena_ptr_t dst_end     = dst_start + count;
    arena_ptr_t src_start   = (arena_ptr_t)src;
    arena_ptr_t src_end     = src_start + count;
    arena_ptr_t arena_start = (arena_ptr_t)arena->base;
    arena_ptr_t arena_end   = arena_start + arena->offset;

    if (dst_start < arena_start || dst_end > arena_end || src_start < arena_start || src_end > arena_end) return false;

    arena_memcpy(dst, src, count);

    return true;
}

static inline void *arena_memset_within(Arena *arena, void *dst, int value, size_t count)
{
    if (!arena || arena->capacity == 0) return NULL;
    if (count == 0) return dst;

    arena_ptr_t dst_start   = (arena_ptr_t)dst;
    arena_ptr_t dst_end     = dst_start + count;
    arena_ptr_t arena_start = (arena_ptr_t)arena->base;
    arena_ptr_t arena_end   = arena_start + arena->offset;

    if (dst_start < arena_start || dst_start >= arena_end) return NULL;
    
    return arena_memset(dst, value, count);
}

_ARENA_FORCE_INLINE long long arena_abs(long long value)
{
    return value < 0 ? ~value + 1 : value;
}

/* Helper macros */
#define arena_alloc_struct(pArena, type)           ((type*)arena_alloc_raw((pArena), sizeof(type), alignof(type)))
#define arena_alloc_array(pArena, size, type)      ((size) == 0 ? NULL : (type*)arena_alloc_raw((pArena), sizeof(type)*size, alignof(type)))
#define arena_alloc_struct_zero(pArena, type)      ((type*)arena_alloc_zero((pArena), sizeof(type), alignof(type)))
#define arena_alloc_array_zero(pArena, size, type) ((size) == 0 ? NULL : (type*)arena_alloc_zero((pArena), sizeof(type)*size, alignof(type)))

#endif // ARENA_IMPLEMENTATION

#endif // ARENA_H_
