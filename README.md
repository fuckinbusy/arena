# Arena Allocator

[Arena Allocator](https://en.wikipedia.org/wiki/Region-based_memory_management) implementation in pure C as an [stb-style single-file library](https://github.com/nothings/stb).

*Project for China University of Petroleum (UPC) Internship.*

## About This Project

This arena allocator was developed as part of an **internship project at China University of Petroleum (UPC)**.

The primary objectives of the project are:

* Study and apply principles of **manual memory management** in C
* Design a **region-based allocation model** with predictable performance
* Reduce allocation overhead and memory fragmentation
* Support multiple **growth strategies** (fixed, realloc-based, chunk-based)
* Provide a **portable, single-header** implementation with minimal dependencies

The allocator targets **performance-sensitive systems** such as compilers, parsers, game engines, and real-time applications, where allocation patterns are well-defined and memory can be released in bulk.

---

## Quick Start

> The truly reusable code is the one that you can simply copy-paste.

The library itself does not require any special building. You can simple copy-paste [./arena.h](./arena.h) to your project and `#include` it.

```c
#define ARENA_IMPLEMENTATION
#include "arena.h"

int main(void)
{
    Arena arena = arena_create(ARENA_CAPACITY_4KB);<img width="1595" height="1118" alt="allocations" src="https://github.com/user-attachments/assets/0b26a21f-d65b-4053-b3bf-db02305f9e1c" />


    int *values = arena_alloc_array(&arena, 100, int);
    for (int i = 0; i < 100; ++i) {
        values[i] = i;
    }

    arena_reset(&arena);   // releases all allocations at once
    arena_destroy(&arena); // frees underlying memory

    return 0;
}
```

# Benchmarks Results
1 Million allocations
<img width="1595" height="1118" alt="allocations" src="https://github.com/user-attachments/assets/c7c84e30-eacf-4935-88db-8e491b57a13f" />

---

Parsing heavy JSON file (10K iterations)
<img width="1704" height="1118" alt="parsing" src="https://github.com/user-attachments/assets/6a6ea9c0-cb2c-404a-8187-0c81ae9679d5" />

---

Rendering pixels on a screen (106666 pixels per frame)
<img width="2990" height="1638" alt="rendering" src="https://github.com/user-attachments/assets/3d353f58-e84e-4fe1-9021-35353c7600a1" />

