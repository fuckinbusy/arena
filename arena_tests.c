#include <stdio.h>
#include <stdbool.h>
#include <time.h>
// #define ARENA_PLATFORM ARENA_PLATFORM_LIBC
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
    ASSERT(arena.base != NULL);
    ASSERT(arena.capacity >= ARENA_CAPACITY_4KB);

    arena_destroy(&arena);
    ASSERT(arena.base == NULL);
    ASSERT(arena.capacity == 0);
    ASSERT(arena.offset == 0);
    
    return true;
}

TEST_CREATE(test_arena_alloc)
{
    Arena arena = arena_create(ARENA_CAPACITY_4KB);
    ASSERT(arena.base != NULL && arena.capacity >= ARENA_CAPACITY_4KB);

    int *a = arena_alloc_struct(&arena, int);
    int *b = arena_alloc_struct(&arena, int);

    ASSERT(a != NULL && b != NULL);
    ASSERT(a < b);

    arena_destroy(&arena);
    ASSERT(arena.base == NULL);

    return true;
}

TEST_CREATE(test_arena_alignment)
{
    Arena arena = arena_create(ARENA_CAPACITY_2KB);
    
    void *pa = arena_alloc(&arena, 1, ARENA_ALIGN_CACHELINE);
    void *pb = arena_alloc(&arena, 13, ARENA_ALIGN_CACHELINE);
    
    ASSERT(((uintptr_t)pa % ARENA_ALIGN_CACHELINE) == 0);
    ASSERT(((uintptr_t)pb % ARENA_ALIGN_CACHELINE) == 0);
    
    arena_destroy(&arena);
    ASSERT(arena.base == NULL);

    return true;
}

TEST_CREATE(test_arena_overflow)
{
    Arena arena = arena_create(ARENA_CAPACITY_2KB);

    void *p = arena_alloc(&arena, 0x4000, ARENA_ALIGN_8B);
    ASSERT(p == NULL);

    arena_destroy(&arena);
    ASSERT(arena.base == NULL);

    return true;
}

TEST_CREATE(test_arena_reset)
{
    Arena arena = arena_create(ARENA_CAPACITY_2KB);

    void *pa = arena_alloc(&arena, 0x400, ARENA_ALIGN_CACHELINE);
    ASSERT(pa != NULL);

    arena_reset(&arena);

    void *pb = arena_alloc(&arena, 0x400, ARENA_ALIGN_CACHELINE);
    ASSERT(pb != NULL);

    ASSERT(pa == pb);

    arena_destroy(&arena);
    ASSERT(arena.base == NULL);

    return true;
}

TEST_CREATE(test_arena_grow_linear)
{
    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1KB,
        ARENA_CAPACITY_8KB,
        ARENA_ALIGN_8B,
        ARENA_GROWTH_CONTRACT_LINEAR,
        1024,
        ARENA_FLAG_NONE
    ));

    ASSERT(arena.capacity >= ARENA_CAPACITY_1KB);
    ASSERT(arena.max_capacity == ARENA_CAPACITY_8KB);

    void *p = arena_alloc(&arena, 2048, ARENA_ALIGN_16B);
    if (!p) {
        ASSERT(arena_grow(&arena, 2048));
        ASSERT(arena.offset == 0);
        ASSERT(arena.capacity > ARENA_CAPACITY_1KB);
        p = arena_alloc(&arena, 2048, ARENA_ALIGN_16B);
    }
    ASSERT(p != NULL);

    arena_reset(&arena);
    ASSERT(arena.offset == 0);

    p = arena_alloc(&arena, 4096, ARENA_ALIGN_8B);
    if (!p) {
        ASSERT(!arena_grow(&arena, 16384));
        ASSERT(arena_grow(&arena, arena.max_capacity));
        ASSERT(arena.capacity == arena.max_capacity);
        ASSERT(!arena_alloc(&arena, arena.max_capacity, ARENA_ALIGN_CACHELINE)); // OOM out of memory
        p = arena_alloc(&arena, (arena.capacity - (ARENA_ALIGN_CACHELINE - 1)), ARENA_ALIGN_CACHELINE);
    }
    ASSERT(p != NULL);

    arena_destroy(&arena);

    return true;
}

TEST_CREATE(test_arena_stress)
{
    const uint32_t ITERATIONS = 1000000;

    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_128MB,
        0,
        ARENA_ALIGN_CACHELINE,
        ARENA_GROWTH_CONTRACT_FIXED,
        ARENA_GROWTH_FACTOR_NONE,
        ARENA_FLAG_DEBUG
    ));

    ASSERT(arena.base != NULL);
    ASSERT(arena.max_capacity == arena.capacity);

    arena_ptr_t arena_start  = (arena_ptr_t)arena.base;
    arena_ptr_t arena_end    = arena_start + arena.capacity;
    arena_size_t last_offset = arena.offset;

    for (int i = 0; i < ITERATIONS; ++i) {
        arena_size_t size = 1 + (randrand() % 0x1000);

        size_t alignments[4] = {
            ARENA_ALIGN_8B,
            ARENA_ALIGN_16B,
            ARENA_ALIGN_32B,
            ARENA_ALIGN_64B, // same as ARENA_ALIGN_CACHELINE
        };
        size_t alignment = alignments[randrand()%4];

        void *p = arena_alloc(&arena, size, alignment);

        if (!p) break;

        arena_ptr_t address = (arena_ptr_t)p;

        ASSERT(address >= arena_start);
        ASSERT(address + size <= arena_end);
        ASSERT((address % alignment) == 0);
        ASSERT(arena.offset >= last_offset);

        arena_memset(p, (int)(i & 0xFF), size);

        last_offset = arena.offset;
    }

    ASSERT(arena.debug.end != NULL);
    ASSERT(arena.debug.total_allocations > 0);
    ASSERT(arena.debug.total_allocations <= arena.capacity);
    
    arena_reset(&arena);

    void *a = arena_alloc(&arena, 64, ARENA_ALIGN_DEFAULT);
    ASSERT(a != NULL);
    
    arena_reset(&arena);
    
    void *b = arena_alloc(&arena, 64, ARENA_ALIGN_DEFAULT);
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
    TEST_RUN(test_arena_grow_linear);
    TEST_RUN(test_arena_stress);
    return 0;
}