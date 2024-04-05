#include "tlb.h"
#include <limits.h>

#define SETS 16
#define WAYS 4

// Define the constant for an invalid address.
#define INVALID_ADDRESS ((size_t)-1)

// Define the cache line structure
typedef struct {
    size_t vpn;       // Virtual Page Number
    size_t pa;        // Physical Address
    int valid;        // Valid bit
    unsigned lru;     // LRU counter
} TLBEntry;

// TLB structure: 16 sets, 4 ways each
static TLBEntry tlb[SETS][WAYS];

// Helper function to clear a single TLB entry
void clear_tlb_entry(TLBEntry *entry) {
    entry->vpn = 0;
    entry->pa = 0;
    entry->valid = 0;
    entry->lru = UINT_MAX;
}

// Initialize or clear the TLB
void tlb_clear() {
    for (int i = 0; i < SETS; ++i) {
        for (int j = 0; j < WAYS; ++j) {
            clear_tlb_entry(&tlb[i][j]);
        }
    }
}

// Function to update the LRU counter
void update_lru(int set, int way) {
    unsigned min_lru = tlb[set][way].lru;
    for (int w = 0; w < WAYS; ++w) {
        if (tlb[set][w].lru < min_lru) {
            tlb[set][w].lru++;
        }
    }
    tlb[set][way].lru = 0;
}

// Peek into the TLB to find LRU status
int tlb_peek(size_t va) {
    size_t vpn = va >> POBITS;
    int set = vpn % SETS;

    for (int way = 0; way < WAYS; ++way) {
        if (tlb[set][way].valid && tlb[set][way].vpn == vpn) {
            return tlb[set][way].lru + 1;
        }
    }
    return 0;
}

// Translate a virtual address using the TLB
size_t tlb_translate(size_t va) {
    size_t vpn = va >> POBITS;
    int set = vpn % SETS;
    size_t page_offset = va & ((1 << POBITS) - 1);
    
    for (int way = 0; way < WAYS; ++way) {
        if (tlb[set][way].valid && tlb[set][way].vpn == vpn) {
            // Cache hit: update LRU and return PA
            update_lru(set, way);
            return (tlb[set][way].pa & ~((1 << POBITS) - 1)) | page_offset;
        }
    }

    // Cache miss: translate and update TLB
    size_t pa = translate(va & ~((1 << POBITS) - 1));
    if (pa == INVALID_ADDRESS) {
        return INVALID_ADDRESS;
    }

    // Find least recently used line
    int lru_way = 0;
    for (int w = 1; w < WAYS; ++w) {
        if (tlb[set][w].lru > tlb[set][lru_way].lru) {
            lru_way = w;
        }
    }

    // Update the LRU line with new data
    tlb[set][lru_way].vpn = vpn;
    tlb[set][lru_way].pa = pa;
    tlb[set][lru_way].valid = 1;
    update_lru(set, lru_way);

    return (pa & ~((1 << POBITS) - 1)) | page_offset;
}
