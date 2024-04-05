#include "tlb.h"
#include <stddef.h>
#include <limits.h> 

typedef struct {
    size_t vpn;      
    size_t pa;      
    int valid;         
    int lru_counter;  
} TLBEntry;

#define TLB_SETS 16
#define TLB_WAYS 4
TLBEntry tlb_cache[TLB_SETS][TLB_WAYS];

void tlb_clear() {
    for (int i = 0; i < TLB_SETS; ++i) {
        for (int j = 0; j < TLB_WAYS; ++j) {
            tlb_cache[i][j].valid = 0;
            tlb_cache[i][j].lru_counter = INT_MAX; 
        }
    }
}

void update_lru(int set_index, int accessed_way) {
    int old_lru = tlb_cache[set_index][accessed_way].lru_counter;
    for (int j = 0; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].valid && tlb_cache[set_index][j].lru_counter < old_lru) {
            tlb_cache[set_index][j].lru_counter++;
        }
    }
    tlb_cache[set_index][accessed_way].lru_counter = 0; 
}

int get_set_index(size_t va) {
    return (va >> POBITS) % TLB_SETS;
}

int tlb_peek(size_t va) {
    size_t vpn = va >> POBITS;
    int set_index = get_set_index(va);

    for (int j = 0; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].valid && tlb_cache[set_index][j].vpn == vpn) {
            // Return LRU status without changing it
            return TLB_WAYS - tlb_cache[set_index][j].lru_counter;
        }
    }
    return 0;
}

size_t tlb_translate(size_t va) {
    size_t vpn = va >> POBITS;
    size_t page_offset = va & ((1 << POBITS) - 1);
    int set_index = get_set_index(va);

    // Check if entry is in TLB
    for (int j = 0; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].valid && tlb_cache[set_index][j].vpn == vpn) {
            update_lru(set_index, j);
            return (tlb_cache[set_index][j].pa | page_offset);
        }
    }
    size_t new_pa = translate(va & ~((1 << POBITS) - 1));
    if (new_pa == (size_t)-1) {
        return (size_t)-1;
    }

    int lru_way = 0, highest_lru = INT_MIN;
    for (int j = 0; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].lru_counter > highest_lru) {
            lru_way = j;
            highest_lru = tlb_cache[set_index][j].lru_counter;
        }
    }

    tlb_cache[set_index][lru_way].vpn = vpn;
    tlb_cache[set_index][lru_way].pa = new_pa;
    tlb_cache[set_index][lru_way].valid = 1;
    update_lru(set_index, lru_way);

    return new_pa | page_offset;
}

