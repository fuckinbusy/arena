#include <stdio.h>
#include <stdbool.h>
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
        printf("\tAssertion failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__);\
        return false;                                                     \
    }                                                                     \
} while (0)

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


int main(void)
{
    TEST_RUN(test_arena_create_destroy);
    TEST_RUN(test_arena_alloc);
    TEST_RUN(test_arena_alignment);
    TEST_RUN(test_arena_overflow);
    TEST_RUN(test_arena_reset);
    TEST_RUN(test_arena_grow_linear);
}