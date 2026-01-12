#include <stdio.h>
#include <stdbool.h>
#include <time.h>
// #define ARENA_PLATFORM ARENA_PLATFORM_LIBC
// #define ARENA_LOGGING
#define ARENA_IMPLEMENTATION
#include "arena.h"

#define TEST_CREATE(name) static bool name(void)
#define TEST_RUN(test)                                             \
do {                                                               \
    printf("[TEST] %-32s ... %s\n", #test, test() ? "OK" : "FAIL");\
} while (0)

#define ASSERT(cond)                                                       \
do {                                                                      \
    if (!(cond)) {                                                          \
        printf("!\tAssertion failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__);\
        return false;                                                     \
    }                                                                     \
} while (0)

/// -------------------------------------
static uint32_t rand_state;

static inline void randinit()
{
    rand_state = (uint32_t)clock();
}

static inline uint32_t randrand()
{
    rand_state = rand_state * 0x333222 + 1013904223u;
    return rand_state;
}
/// -------------------------------------

TEST_CREATE(test_arena_create_destroy)
{
    Arena arena = arena_create(ARENA_CAPACITY_4KB);
    ASSERT(arena.last_chunk != NULL);
    ASSERT(arena.last_chunk->capacity >= ARENA_CAPACITY_4KB);

    arena_destroy(&arena);
    ASSERT(arena.last_chunk == NULL);
    // ASSERT(arena.commited == 0);
    
    return true;
}

TEST_CREATE(test_arena_alloc)
{
    Arena arena = arena_create(ARENA_CAPACITY_4KB);
    ASSERT(arena.last_chunk != NULL);
    ASSERT(arena.last_chunk->capacity >= ARENA_CAPACITY_4KB);

    int *a = arena_alloc_struct(&arena, int);
    int *b = arena_alloc_struct(&arena, int);

    ASSERT(a != NULL && b != NULL);
    ASSERT(a < b);

    arena_destroy(&arena);
    ASSERT(arena.last_chunk == NULL);

    return true;
}

TEST_CREATE(test_arena_alignment)
{
    Arena arena = arena_create(ARENA_CAPACITY_1KB);
    ASSERT(arena.last_chunk != NULL);
    ASSERT(arena.last_chunk->capacity >= ARENA_CAPACITY_1KB);

    void *pa = arena_alloc_raw(&arena, 1, ARENA_ALIGN_CACHELINE);
    void *pb = arena_alloc_raw(&arena, 13, ARENA_ALIGN_CACHELINE);
    void *pc = arena_alloc_raw(&arena, 64, ARENA_ALIGN_CACHELINE);

    ASSERT(pa != NULL && pb != NULL && pc != NULL);
    ASSERT(((uintptr_t)pa % ARENA_ALIGN_CACHELINE) == 0);
    ASSERT(((uintptr_t)pb % ARENA_ALIGN_CACHELINE) == 0);
    ASSERT(((uintptr_t)pc % ARENA_ALIGN_CACHELINE) == 0);
    
    arena_destroy(&arena);
    ASSERT(arena.last_chunk == NULL);

    return true;
}

TEST_CREATE(test_arena_overflow)
{
    Arena arena = arena_create(ARENA_CAPACITY_2KB);
    ASSERT(arena.last_chunk != NULL);
    ASSERT(arena.last_chunk->capacity >= ARENA_CAPACITY_2KB);

    void *p = arena_alloc_raw(&arena, 0x4000, ARENA_ALIGN_8B);
    ASSERT(p == NULL);
    ASSERT(arena.error == ARENA_ERROR_OOM);

    arena_destroy(&arena);
    ASSERT(arena.last_chunk == NULL);

    return true;
}

TEST_CREATE(test_arena_reset)
{
    Arena arena = arena_create(ARENA_CAPACITY_2KB);
    ASSERT(arena.last_chunk != NULL);
    ASSERT(arena.last_chunk->capacity >= ARENA_CAPACITY_2KB);

    void *pa = arena_alloc_raw(&arena, 0x400, ARENA_ALIGN_CACHELINE);
    ASSERT(pa != NULL);
    ASSERT(arena.last_chunk->offset > 0);

    arena_reset(&arena);
    ASSERT(arena.last_chunk->offset == 0);

    void *pb = arena_alloc_raw(&arena, 0x400, ARENA_ALIGN_CACHELINE);
    ASSERT(pb != NULL);

    ASSERT(pa == pb);

    arena_destroy(&arena);
    ASSERT(arena.last_chunk == NULL);

    return true;
}

TEST_CREATE(test_arena_reset_chunky)
{
    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1KB,
        ARENA_CAPACITY_8KB,
        ARENA_GROWTH_CONTRACT_CHUNKY,
        ARENA_GROWTH_FACTOR_CHUNKY_4KB,
        ARENA_FLAG_NONE
    ));
    
    ASSERT(arena.last_chunk != NULL);
    ASSERT(arena.last_chunk->capacity == ARENA_CAPACITY_1KB);
    ASSERT(arena.max_capacity >= ARENA_CAPACITY_8KB);

    void *pa = arena_alloc_raw(&arena, 0x800, ARENA_ALIGN_CACHELINE);
    ASSERT(pa != NULL);
    ASSERT(arena.head_chunk->next != NULL);
    ASSERT(arena.reserved == ARENA_CAPACITY_1KB + ARENA_CAPACITY_4KB);
    
    arena_reset(&arena);
    ASSERT(arena.epoch > 0);
    ASSERT(arena.head_chunk->offset == 0);
    ASSERT(arena.last_chunk->offset == 0);
    
    void *pb = arena_alloc_raw(&arena, 0x800, ARENA_ALIGN_CACHELINE);
    ASSERT(pb != NULL);
    ASSERT(pa == pb);
    
    arena_destroy(&arena);
    ASSERT(arena.last_chunk == NULL);
    
    return true;
}

TEST_CREATE(test_arena_grow_chunky)
{
    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1KB,
        ARENA_CAPACITY_8KB,
        ARENA_GROWTH_CONTRACT_CHUNKY,
        ARENA_GROWTH_FACTOR_CHUNKY_2KB,
        ARENA_FLAG_DEBUG
    ));
    
    void *pa = arena_alloc_raw(&arena, 0x200, ARENA_ALIGN_4B);
    ASSERT(pa != NULL);
    ASSERT(arena.head_chunk->next == NULL); // no new chunks here
    
    void *pb = arena_alloc_raw(&arena, 0x400, ARENA_ALIGN_4B); 
    ASSERT(pb != NULL);
    ASSERT(arena.head_chunk->next != NULL); // +1 new chunk
    
    void *pc = arena_alloc_raw(&arena, 0x1000, ARENA_ALIGN_4B);
    ASSERT(pc != NULL);
    ASSERT(arena.head_chunk->next->next != NULL); // +1 new chunk
    
    ASSERT(arena.reserved == ARENA_CAPACITY_8KB);

    pc = arena_alloc_raw(&arena, 0x10F0, ARENA_ALIGN_4B);
    ASSERT(pc == NULL);    
    ASSERT(arena.error == ARENA_ERROR_OOM);

    arena_destroy(&arena);
    return true;
}

TEST_CREATE(test_arena_memory_resolve) {
    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1KB,
        ARENA_CAPACITY_8KB,
        ARENA_GROWTH_CONTRACT_REALLOC,
        ARENA_GROWTH_FACTOR_REALLOC_2X,
        ARENA_FLAG_DEBUG
    ));

    ArenaMemory mem = arena_alloc(&arena, 731, 4);
    ASSERT(mem.data != NULL);
    *(int*)mem.data= 0x1F;
    ASSERT(*(int*)mem.data == 0x1F);
    ASSERT(mem.size == 731);
    
    arena_size_t old_offset = arena.last_chunk->offset;
    
    ArenaMemory mem2 = arena_alloc(&arena, 999, 4);
    ASSERT(mem2.data != NULL);
    *(int*)mem2.data = 0xF1;
    ASSERT(*(int*)mem2.data == 0xF1);
    ASSERT(mem2.size == 999);

    ASSERT(arena.last_chunk == arena.head_chunk);
    
    ASSERT(arena_memory_resolve(&arena, &mem));
    ASSERT(arena_memory_resolve(&arena, &mem2));
    
    ASSERT(*(int*)mem.data == 0x1F);

    ArenaMemory mem3 = arena_alloc(&arena, 1024, 8);
    ASSERT(mem3.data != NULL);
    ASSERT(*(int*)mem2.data == 0xF1);

    arena_destroy(&arena);

    return true;
}

TEST_CREATE(test_arena_error) {
    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1KB,
        ARENA_CAPACITY_8KB,
        ARENA_GROWTH_CONTRACT_FIXED,
        ARENA_GROWTH_FACTOR_NONE,
        ARENA_FLAG_DEBUG
    ));

    ASSERT(arena.last_chunk != NULL);

    ASSERT(arena_alloc_raw(&arena, 2048, 7) == NULL);
    ASSERT(arena.error == ARENA_ERROR_INVALID_ALIGNMENT);
    
    ASSERT(arena.last_chunk->offset == 0);

    ASSERT(arena_alloc_raw(&arena, 0x3000, 8) == NULL);
    ASSERT(arena.error == ARENA_ERROR_OOM);

    arena_destroy(&arena);

    ASSERT(arena.error == ARENA_ERROR_NONE);

    arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1KB,
        ARENA_CAPACITY_1MB,
        ARENA_GROWTH_CONTRACT_REALLOC,
        ARENA_GROWTH_FACTOR_REALLOC_2X,
        ARENA_FLAG_DEBUG
    ));

    ASSERT(arena.last_chunk != NULL);

    ASSERT(arena_alloc_raw(&arena, 2048, 7) == NULL);
    ASSERT(arena.error == ARENA_ERROR_INVALID_ALIGNMENT);

    ASSERT(arena_alloc_raw(&arena, 2048, 8) != NULL);
    ASSERT(arena.error == ARENA_ERROR_NONE);

    arena_destroy(&arena);

    return true;
}

TEST_CREATE(test_arena_stress_no_grow)
{
    const uint32_t ITERATIONS = 1000000;

    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_128MB,
        0,
        ARENA_GROWTH_CONTRACT_FIXED,
        ARENA_GROWTH_FACTOR_NONE,
        ARENA_FLAG_DEBUG
    ));

    ASSERT(arena.last_chunk != NULL);
    ASSERT(arena.max_capacity == arena.reserved);

    arena_ptr_t arena_start  = (arena_ptr_t)arena.last_chunk;
    arena_ptr_t arena_end    = arena_start + arena.reserved;
    arena_size_t last_offset = arena.last_chunk->offset;

    for (int i = 0; i < ITERATIONS; ++i) {
        arena_size_t size = 1 + (randrand() % 0x1000);

        size_t alignments[4] = {
            ARENA_ALIGN_8B,
            ARENA_ALIGN_16B,
            ARENA_ALIGN_32B,
            ARENA_ALIGN_64B, // same as ARENA_ALIGN_CACHELINE
        };
        size_t alignment = alignments[randrand()%4];

        void *p = arena_alloc_raw(&arena, size, alignment);

        if (!p) break;

        arena_ptr_t address = (arena_ptr_t)p;

        ASSERT(address >= arena_start);
        ASSERT(address + size <= arena_end);
        ASSERT((address % alignment) == 0);
        ASSERT(arena.last_chunk->offset >= last_offset);

        arena_memset(p, (int)(i & 0xFF), size);

        last_offset = arena.last_chunk->offset;
    }

    ASSERT(arena.debug.end != NULL);
    ASSERT(arena.debug.total_allocations > 0);
    ASSERT(arena.debug.total_allocations <= arena.reserved);
    
    arena_reset(&arena);

    void *a = arena_alloc_raw(&arena, 64, ARENA_ALIGN_DEFAULT);
    ASSERT(a != NULL);
    
    arena_reset(&arena);
    
    void *b = arena_alloc_raw(&arena, 64, ARENA_ALIGN_DEFAULT);
    ASSERT(b != NULL);
    
    ASSERT(b == a);

    arena_destroy(&arena);

    return true;
}

int main(void)
{
    randinit();
    TEST_RUN(test_arena_create_destroy);
    TEST_RUN(test_arena_alloc);
    TEST_RUN(test_arena_alignment);
    TEST_RUN(test_arena_overflow);
    TEST_RUN(test_arena_reset);
    TEST_RUN(test_arena_reset_chunky);
    TEST_RUN(test_arena_grow_chunky);
    TEST_RUN(test_arena_memory_resolve);
    TEST_RUN(test_arena_error);
    TEST_RUN(test_arena_stress_no_grow);
    return 0;
}