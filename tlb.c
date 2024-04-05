#include "tlb.h"
#include <stddef.h>

// Define a structure for TLB entries
typedef struct {
    size_t vpn;        // Virtual Page Number
    size_t pa;         // Physical Address
    int valid;         // Valid bit
    int lru_counter;   // LRU counter
} TLBEntry;

// TLB as a 4-way set-associative cache with 16 sets
#define TLB_SETS 16
#define TLB_WAYS 4
TLBEntry tlb_cache[TLB_SETS][TLB_WAYS];

// Initialize the TLB
void tlb_clear() {
    for (int i = 0; i < TLB_SETS; ++i) {
        for (int j = 0; j < TLB_WAYS; ++j) {
            tlb_cache[i][j].valid = 0;
            tlb_cache[i][j].lru_counter = 0;
        }
    }
}

// Helper function to update LRU counters
void update_lru(int set_index, int used_way) {
    for (int j = 0; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].valid && j != used_way) {
            tlb_cache[set_index][j].lru_counter++;
        }
    }
    tlb_cache[set_index][used_way].lru_counter = 0;
}

// Helper function to get the LRU way index in a set
int get_lru_way(int set_index) {
    int lru_way = 0, max_lru = tlb_cache[set_index][0].lru_counter;

    for (int j = 1; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].lru_counter > max_lru) {
            lru_way = j;
            max_lru = tlb_cache[set_index][j].lru_counter;
        }
    }
    return lru_way;
}

// Helper function to get the set index from a virtual address
int get_set_index(size_t va) {
    // Calculate set index based on VA. This depends on how VA is structured.
    // This is a placeholder implementation.
    return (va >> POBITS) % TLB_SETS;
}

// Peek into TLB for a given VA
int tlb_peek(size_t va) {
    size_t vpn = va >> POBITS;
    int set_index = get_set_index(va);

    for (int j = 0; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].valid && tlb_cache[set_index][j].vpn == vpn) {
            return tlb_cache[set_index][j].lru_counter + 1;
        }
    }

    return 0; // Not found in TLB
}

// Translate a VA using the TLB
size_t tlb_translate(size_t va) {
    size_t vpn = va >> POBITS;
    int set_index = get_set_index(va);
    size_t page_offset = va & ((1 << POBITS) - 1);

    for (int j = 0; j < TLB_WAYS; ++j) {
        if (tlb_cache[set_index][j].valid && tlb_cache[set_index][j].vpn == vpn) {
            update_lru(set_index, j);
            return (tlb_cache[set_index][j].pa | page_offset);
        }
    }

    // Not found in TLB, translate and update
    size_t new_pa = translate(va & ~((1 << POBITS) - 1));
    if (new_pa == (size_t)-1) {
        return (size_t)-1;
    }

    // Find the LRU way for replacement
    int lru_way = get_lru_way(set_index);
    tlb_cache[set_index][lru_way].vpn = vpn;
    tlb_cache[set_index][lru_way].pa = new_pa;
    tlb_cache[set_index][lru_way].valid = 1;
    update_lru(set_index, lru_way);

    return new_pa | page_offset;
}
