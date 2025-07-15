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
void* operator new(size_t size, std::align_val_t a) {
    return kmalloc(size, static_cast<size_t>(a)); 
}
void* operator new[](size_t size, std::align_val_t a) { 
    return kmalloc(size, static_cast<size_t>(a)); 

}
void operator delete(void* ptr) noexcept {}
void operator delete[](void* ptr) noexcept {}
void operator delete(void* ptr, size_t) noexcept {}
void operator delete[](void* ptr, size_t) noexcept {}

alignas(0x1000) static PageTable pml4;
constexpr uint64_t ADDR_MASK = 0x0000FFFF'FFFFF000ULL;
constexpr uint64_t FLAGS_MASK = (1ULL<<63) | 0xFFFULL;
constexpr uint64_t PAGE_4K       = 0x1000;
constexpr uint64_t PAGE_2M       = 0x200000;               
constexpr uint64_t PAGE_1G       = 0x40000000;

constexpr uint64_t KERNEL_BASE = 0xFFFF'8000'0000'0000ULL;
uint64_t virt_to_phys(uint64_t vaddr, uint64_t* oldPML4);


uint64_t virt_to_phys(uint64_t vaddr, uint64_t* oldPML4)
{
    // -------------------------------------------------------------------------
    // 1. Extract indices for each paging level
    // -------------------------------------------------------------------------
    uint16_t pml4_i = (vaddr >> 39) & 0x1FF; // top 9 bits
    uint16_t pdpt_i = (vaddr >> 30) & 0x1FF;
    uint16_t pd_i   = (vaddr >> 21) & 0x1FF;
    uint16_t pt_i   = (vaddr >> 12) & 0x1FF;
    uint64_t page_off = vaddr & 0xFFF;

    // -------------------------------------------------------------------------
    // 2. Walk the hierarchy (identity‑mapped tables → simple pointer cast)
    // -------------------------------------------------------------------------
    PageTable* pml4 = reinterpret_cast<PageTable*>(oldPML4);

    uint64_t pdpte = (*pml4)[pml4_i];
    if (!(pdpte & PAGE_PRESENT)) return 0;

    if (pdpte & PAGE_PSE) {                     // 1 GiB page
        uint64_t phys = (pdpte & ADDR_MASK) + (vaddr & (PAGE_1G - 1));
        return phys;
    }

    PageTable* pdpt = reinterpret_cast<PageTable*>(pdpte & ADDR_MASK);
    uint64_t pde = (*pdpt)[pdpt_i];
    if (!(pde & PAGE_PRESENT)) return 0;

    if (pde & PAGE_PSE) {                       // 2 MiB page
        uint64_t phys = (pde & ADDR_MASK) + (vaddr & (PAGE_2M - 1));
        return phys;
    }

    PageTable* pd = reinterpret_cast<PageTable*>(pde & ADDR_MASK);
    uint64_t pte = (*pd)[pd_i];
    if (!(pte & PAGE_PRESENT)) return 0;

    PageTable* pt = reinterpret_cast<PageTable*>(pte & ADDR_MASK);
    uint64_t pe = (*pt)[pt_i];
    if (!(pe & PAGE_PRESENT)) return 0;

    // 4 KiB mapping
    return (pe & ADDR_MASK) + page_off;
}

void setup_paging(uint64_t* PML4Address) {
    memcpy(&pml4, PML4Address, sizeof(PageTable));

    for(int i = 0; i < 256; i++) {
        pml4[i] = 0; // clear lower half
    }

    for(int i = 256; i < 512; i++) {
        uint64_t pdpt = pml4[i] & ADDR_MASK;
        uint64_t pdpt_flags = pml4[i] & FLAGS_MASK;
        
        if((pdpt_flags & PAGE_PRESENT) && !(pdpt_flags & PAGE_PSE)) {
            uint64_t* new_pdpt = (uint64_t*)kmalloc(sizeof(PageTable), 4096);
            memcpy(new_pdpt, (uint64_t*)pdpt, sizeof(PageTable));
            pml4[i] = virt_to_phys((uint64_t)new_pdpt, PML4Address) | pdpt_flags;

            for(int j = 0; j < 512; j++) {
                uint64_t pd = new_pdpt[j] & ADDR_MASK;
                uint64_t pd_flags = new_pdpt[j] & FLAGS_MASK;

                if((pd_flags & PAGE_PRESENT) && !(pd_flags & PAGE_PSE)) {
                    uint64_t* new_pd = (uint64_t*)kmalloc(sizeof(PageTable), 4096);
                    memcpy(new_pd, (uint64_t*)pd, sizeof(PageTable));
                    new_pdpt[j] = virt_to_phys((uint64_t)new_pd, PML4Address) | pd_flags;

                    for(int k = 0; k < 512; k++) {
                        uint64_t pt = new_pd[k] & ADDR_MASK;
                        uint64_t pt_flags = new_pd[k] & FLAGS_MASK;

                        if((pt_flags & PAGE_PRESENT) && !(pt_flags & PAGE_PSE)) {
                            uint64_t* new_pt = (uint64_t*)kmalloc(sizeof(PageTable), 4096);
                            memcpy(new_pt, (uint64_t*)pt, sizeof(PageTable));
                            new_pd[k] = virt_to_phys((uint64_t)new_pt, PML4Address) | pt_flags;
                        }

                    }

                }

            }

        }

    }

    load_cr3(virt_to_phys((uint64_t)pml4, PML4Address));

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
        uint64_t va = (virt + i * PAGE_SIZE_4K) & ~0xFFF;
        uint64_t pa = (phys + i * PAGE_SIZE_4K) & ~0xFFF;

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
        uint64_t va = (virt + i * PAGE_SIZE_4K) & ~0xFFF;
        uint64_t pa = (phys + i * PAGE_SIZE_4K) & ~0xFFF;

        uint16_t pml4_i = (va >> 39) & 0x1FF;
        uint16_t pdpt_i = (va >> 30) & 0x1FF;
        uint16_t pd_i   = (va >> 21) & 0x1FF;

        PageTable* pdpt_tbl = get_or_create_table(&pml4, pml4_i);
        PageTable* pd_tbl   = get_or_create_table(pdpt_tbl, pdpt_i);

        (*pd_tbl)[pd_i] = pa | PAGE_PRESENT | PAGE_RW | PAGE_PSE;
    }
}

PageTable* get_table_if_exists(PageTable* parent, uint16_t index) {
    if (!( (*parent)[index] & PAGE_PRESENT ))
        return nullptr;

    return reinterpret_cast<PageTable*>((*parent)[index] & ~0xFFFULL);
}

inline void flush_tlb_single(uint64_t va) {
    asm volatile ("invlpg (%0)" : : "r" (va) : "memory");
}

void unmap_4k(uint64_t virt, uint64_t count) {
    for (uint64_t i = 0; i < count; ++i) {
        uint64_t va = (virt + i * PAGE_SIZE_4K) & ~0xFFF;

        uint16_t pml4_i = (va >> 39) & 0x1FF;
        uint16_t pdpt_i = (va >> 30) & 0x1FF;
        uint16_t pd_i   = (va >> 21) & 0x1FF;
        uint16_t pt_i   = (va >> 12) & 0x1FF;

        PageTable* pdpt_tbl = get_table_if_exists(&pml4, pml4_i);
        if (!pdpt_tbl) continue;

        PageTable* pd_tbl = get_table_if_exists(pdpt_tbl, pdpt_i);
        if (!pd_tbl) continue;

        PageTable* pt_tbl = get_table_if_exists(pd_tbl, pd_i);
        if (!pt_tbl) continue;

        (*pt_tbl)[pt_i] = 0;  
        flush_tlb_single(virt);
    }
}

void unmap_2m(uint64_t virt, uint64_t count) {
    for (uint64_t i = 0; i < count; ++i) {
        uint64_t va = (virt + i * PAGE_SIZE_4K) & ~0xFFF;

        uint16_t pml4_i = (va >> 39) & 0x1FF;
        uint16_t pdpt_i = (va >> 30) & 0x1FF;
        uint16_t pd_i   = (va >> 21) & 0x1FF;

        PageTable* pdpt_tbl = get_table_if_exists(&pml4, pml4_i);
        if (!pdpt_tbl) continue;

        PageTable* pd_tbl = get_table_if_exists(pdpt_tbl, pdpt_i);
        if (!pd_tbl) continue;

        (*pd_tbl)[pd_i] = 0;  
        flush_tlb_single(virt);
    }
}
