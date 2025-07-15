#include <stddef.h>
#include <stdint.h>
#include "memory.hpp"

static uint8_t* heap_base = &__heap_start;
static size_t heap_size = &__heap_end - &__heap_start;
static size_t heap_offset = 0;

void* kmalloc(size_t size, size_t align) {
    size_t current = reinterpret_cast<size_t>(heap_base) + heap_offset;
    size_t aligned = (current + align - 1) & ~(align - 1);
    size_t next_offset = aligned - reinterpret_cast<size_t>(heap_base) + size;

    if (next_offset >= heap_size) {
        return nullptr; // Out of memory
    }

    heap_offset = next_offset;
    return reinterpret_cast<void*>(aligned);
}

void* operator new(size_t size) {
    return kmalloc(size);
}
void* operator new[](size_t size) {
    return kmalloc(size);
}
void operator delete(void* ptr) noexcept {}
void operator delete[](void* ptr) noexcept {}
void operator delete(void* ptr, size_t) noexcept {}
void operator delete[](void* ptr, size_t) noexcept {}

alignas(0x1000) PageTable pml4;
alignas(0x1000) PageTable pdpt;
alignas(0x1000) PageTable pd;
alignas(0x1000) PageTable pt;

void setup_paging() {
    // Clear all
    memset(pml4, 0, sizeof(pml4));
    memset(pdpt, 0, sizeof(pdpt));
    memset(pd,   0, sizeof(pd));
    memset(pt,   0, sizeof(pt));

    // Link tables
    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_RW;
    pdpt[0] = (uint64_t)pd   | PAGE_PRESENT | PAGE_RW;
    pd[0]   = (uint64_t)pt   | PAGE_PRESENT | PAGE_RW;

    // Identity map first 1MiB using 4KiB pages
    for (uint64_t i = 0; i < 0x100000; i += PAGE_SIZE_4K) {
        pt[i / PAGE_SIZE_4K] = i | PAGE_PRESENT | PAGE_RW;
    }

    // Load CR3 and enable paging
    load_cr3((uint64_t)pml4);

    // You also need to enable paging + long mode from assembly:
    //   - Set CR4.PAE = 1
    //   - Set EFER.LME = 1 (MSR 0xC0000080)
    //   - Set CR0.PG = 1
}

PageTable* getPML4(){
    return &pml4;
}

PageTable* get_or_create_table(PageTable* parent, uint16_t index) {
    if (!((*parent)[index] & PAGE_PRESENT)) {
        PageTable* new_table = (PageTable*)kmalloc(sizeof(PageTable), 4096);
        memset(new_table, 0, 4096);
        (*parent)[index] = (uint64_t)new_table | PAGE_PRESENT | PAGE_RW;
        return new_table;
    } else {
        return (PageTable*)((*parent)[index] & ~0xFFF);
    }
}

void map_4k(uint64_t virt, uint64_t phys, uint64_t count) {
    for (uint64_t i = 0; i < count; ++i) {
        uint64_t va = virt + i * PAGE_SIZE_4K;
        uint64_t pa = phys + i * PAGE_SIZE_4K;

        uint16_t pml4_i = (va >> 39) & 0x1FF;
        uint16_t pdpt_i = (va >> 30) & 0x1FF;
        uint16_t pd_i   = (va >> 21) & 0x1FF;
        uint16_t pt_i   = (va >> 12) & 0x1FF;

        PageTable* pdpt_tbl = get_or_create_table(&pml4, pml4_i);
        PageTable* pd_tbl   = get_or_create_table(pdpt_tbl, pdpt_i);
        PageTable* pt_tbl   = get_or_create_table(pd_tbl, pd_i);

        (*pt_tbl)[pt_i] = pa | PAGE_PRESENT | PAGE_RW;
    }
}

void map_2m(uint64_t virt, uint64_t phys, uint64_t count) {
    for (uint64_t i = 0; i < count; ++i) {
        uint64_t va = virt + i * PAGE_SIZE_2M;
        uint64_t pa = phys + i * PAGE_SIZE_2M;

        uint16_t pml4_i = (va >> 39) & 0x1FF;
        uint16_t pdpt_i = (va >> 30) & 0x1FF;
        uint16_t pd_i   = (va >> 21) & 0x1FF;

        PageTable* pdpt_tbl = get_or_create_table(&pml4, pml4_i);
        PageTable* pd_tbl   = get_or_create_table(pdpt_tbl, pdpt_i);

        (*pd_tbl)[pd_i] = pa | PAGE_PRESENT | PAGE_RW | PAGE_PSE;
    }
}