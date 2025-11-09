// Content from adaptive_cache.cc
////////////////////////////////////////////
//                                        //
//  Adaptive Cache Replacement Policy     //
//  A novel hybrid policy combining       //
//  SRRIP, SHIP, and ReD concepts         //
//  John Doe, johndoe@example.com         //
//                                        //
////////////////////////////////////////////

#include <iostream>
#include <cstdlib>
#include "../inc/champsim_crc2.h"

using namespace std;

#define NUM_CORE 4
#define MAX_LLC_SETS 8192
#define LLC_WAYS 16

#define SAT_INC(x,max)  ((x)<(max)?(x)+1:(x))
#define SAT_DEC(x)      ((x)>0?(x)-1:(x))

#define maxRRPV 3
uint32_t rrpv[MAX_LLC_SETS][LLC_WAYS];
uint32_t is_prefetch[MAX_LLC_SETS][LLC_WAYS];
uint32_t fill_core[MAX_LLC_SETS][LLC_WAYS];

#define NUM_LEADER_SETS   64
uint32_t ship_sample[MAX_LLC_SETS];
uint32_t line_reuse[MAX_LLC_SETS][LLC_WAYS];
uint64_t line_sig[MAX_LLC_SETS][LLC_WAYS];

#define maxSHCTR 7
#define SHCT_SIZE (1<<14)
uint32_t SHCT[NUM_CORE][SHCT_SIZE];

// Avoid macro conflict by renaming the constant from NUM_TYPES to NUM_CACHE_TYPES
const uint32_t NUM_CACHE_TYPES = 4;
uint64_t insertion_distrib[NUM_CACHE_TYPES][maxRRPV+1];
uint64_t total_prefetch_downgrades;

#define RED_SETS_BITS 9
#define RED_WAYS 16
#define RED_TAG_SIZE_BITS 11
#define RED_SECTOR_SIZE_BITS 2
#define RED_PC_FRACTION 4
#define PCs_BITS 8

#define RED_SETS (1<<RED_SETS_BITS)
#define RED_TAG_SIZE (1<<RED_TAG_SIZE_BITS)
#define RED_SECTOR_SIZE (1<<RED_SECTOR_SIZE_BITS)
#define PCs (1<<PCs_BITS)

struct ReD_ART_bl {
    uint64_t tag;
    uint8_t valid[RED_SECTOR_SIZE];
};

struct ReD_ART_set {
    struct ReD_ART_bl adds[RED_WAYS];
    uint32_t insert;
};

struct ReD_ART_set ReD_ART[NUM_CORE][RED_SETS];

struct ReD_ART_PCbl {
    uint64_t pc_entry[RED_SECTOR_SIZE];
};

struct ReD_ART_PCbl ReD_ART_PC[NUM_CORE][RED_SETS/RED_PC_FRACTION][RED_WAYS];

struct ReD_PCRT_bl {
    uint32_t reuse_counter, noreuse_counter;
};

struct ReD_PCRT_bl ReD_PCRT[NUM_CORE][PCs];
uint32_t misses[NUM_CORE];

const uint8_t TRUE = 1;
const uint8_t FALSE = 0;

uint8_t lookup(uint32_t cpu, uint64_t PCnow, uint64_t block);
void remember(uint32_t cpu, uint64_t PCnow, uint64_t block);

void InitReplacementState() {
    int LLC_SETS = (get_config_number() <= 2) ? 2048 : MAX_LLC_SETS;

    cout << "Initialize Adaptive Cache state" << endl;

    for (int i=0; i<MAX_LLC_SETS; i++) {
        for (int j=0; j<LLC_WAYS; j++) {
            rrpv[i][j] = maxRRPV;
            line_reuse[i][j] = FALSE;
            is_prefetch[i][j] = FALSE;
            line_sig[i][j] = 0;
        }
    }

    for (int i=0; i<NUM_CORE; i++) {
        for (int j=0; j<SHCT_SIZE; j++) {
            SHCT[i][j] = 1;
        }
    }

    int leaders=0;
    while(leaders<NUM_LEADER_SETS){
        int randval = rand()%LLC_SETS;
        if(ship_sample[randval]==0){
            ship_sample[randval]=1;
            leaders++;
        }
    }

    for (int core=0; core<NUM_CORE; core++) {
        for (int i=0; i<RED_SETS; i++) {
            ReD_ART[core][i].insert = 0;
            for (int j=0; j<RED_WAYS; j++) {
                ReD_ART[core][i].adds[j].tag=0;
                for (int k=0; k<RED_SECTOR_SIZE; k++) {
                    ReD_ART[core][i].adds[j].valid[k] = 0;
                }
            }
        }
    }

    for (int core=0; core<NUM_CORE; core++) {
        for (int i=0; i<RED_SETS/RED_PC_FRACTION; i++) {
            for (int j=0; j<RED_WAYS; j++) {
                for (int k=0; k<RED_SECTOR_SIZE; k++) {
                    ReD_ART_PC[core][i][j].pc_entry[k] = 0;
                }
            }
        }
    }

    for (int core=0; core<NUM_CORE; core++) {
        for (int i=0; i<PCs; i++) {
            ReD_PCRT[core][i].reuse_counter = 3;
            ReD_PCRT[core][i].noreuse_counter = 0;
        }
    }

    for (int i=0; i<NUM_CORE; i++) {
        misses[i]=0;
    }
}

uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type) {
    uint8_t present;
    uint64_t PCentry;
    uint64_t block = paddr >> 6;

    PCentry = (PC >> 2) & (PCs-1);

    if (type == LOAD || type == RFO || type == PREFETCH) {
        present = lookup(cpu, PC, block);
        if (!present) {
            if ((ReD_PCRT[cpu][PCentry].reuse_counter * 64 > ReD_PCRT[cpu][PCentry].noreuse_counter
                && ReD_PCRT[cpu][PCentry].reuse_counter * 3 < ReD_PCRT[cpu][PCentry].noreuse_counter)
                || (misses[cpu] % 8 == 0)) {
                remember(cpu, PC, block);
            }
            if (ReD_PCRT[cpu][PCentry].reuse_counter * 3 < ReD_PCRT[cpu][PCentry].noreuse_counter) {
                return LLC_WAYS;
            }
        }
    }

    while (1) {
        for (int i=0; i<LLC_WAYS; i++)
            if (rrpv[set][i] == maxRRPV)
                return i;

        for (int i=0; i<LLC_WAYS; i++)
            rrpv[set][i]++;
    }

    return 0;
}

void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit) {
    uint32_t sig = line_sig[set][way];

    if (hit) {
        if (type != WRITEBACK) {
            rrpv[set][way] = 0;
            if (is_prefetch[set][way]) {
                rrpv[set][way] = maxRRPV;
                is_prefetch[set][way] = FALSE;
                total_prefetch_downgrades++;
            }
            if (ship_sample[set] == 1 && line_reuse[set][way] == 0) {
                uint32_t fill_cpu = fill_core[set][way];
                SHCT[fill_cpu][sig] = SAT_INC(SHCT[fill_cpu][sig], maxSHCTR);
                line_reuse[set][way] = TRUE;
            }
        }
        return;
    }

    uint64_t use_PC = (type == PREFETCH) ? ((PC << 1) + 1) : (PC << 1);
    uint32_t new_sig = use_PC % SHCT_SIZE;

    if (ship_sample[set] == 1) {
        uint32_t fill_cpu = fill_core[set][way];
        if (line_reuse[set][way] == FALSE) {
            SHCT[fill_cpu][sig] = SAT_DEC(SHCT[fill_cpu][sig]);
        } else {
            SHCT[fill_cpu][sig] = SAT_INC(SHCT[fill_cpu][sig], maxSHCTR);
        }
        line_reuse[set][way] = FALSE;
        line_sig[set][way] = new_sig;
        fill_core[set][way] = cpu;
    }

    is_prefetch[set][way] = (type == PREFETCH);

    uint32_t priority_RRPV = maxRRPV - 1;

    if (type == WRITEBACK) {
        rrpv[set][way] = maxRRPV;
    } else if (SHCT[cpu][new_sig] == 0) {
        rrpv[set][way] = (rand() % 100 >= 5) ? maxRRPV : priority_RRPV;
    } else if (SHCT[cpu][new_sig] == 7) {
        rrpv[set][way] = (type == PREFETCH) ? 1 : 0;
    } else {
        rrpv[set][way] = priority_RRPV;
    }

    insertion_distrib[type][rrpv[set][way]]++;
}

void PrintStats_Heartbeat() {}

void PrintStats() {
    cout << "Insertion Distribution: " << endl;
    for (uint32_t i=0; i<NUM_CACHE_TYPES; i++) {
        cout << "\t" << "Type " << i << " ";
        for (uint32_t v=0; v<maxRRPV+1; v++) {
            cout << insertion_distrib[i][v] << " ";
        }
        cout << endl;
    }

    cout << "Total Prefetch Downgrades: " << total_prefetch_downgrades << endl;
}

uint8_t lookup(uint32_t cpu, uint64_t PCnow, uint64_t block) {
    uint64_t i, tag, subsector;
    uint64_t ART_set;

    subsector = block & (RED_SECTOR_SIZE-1);
    ART_set = (block >> RED_SECTOR_SIZE_BITS) & (RED_SETS-1);
    tag = (block >> (RED_SETS_BITS + RED_SECTOR_SIZE_BITS)) & (RED_TAG_SIZE-1);

    misses[cpu]++;

    for (i = 0; i < RED_WAYS; i++) {
        if ((ReD_ART[cpu][ART_set].adds[i].tag == tag) && (ReD_ART[cpu][ART_set].adds[i].valid[subsector] == 1)) {
            if (ART_set % RED_PC_FRACTION == 0) {
                uint64_t PCin_entry = ReD_ART_PC[cpu][ART_set / RED_PC_FRACTION][i].pc_entry[subsector];
                ReD_PCRT[cpu][PCin_entry].reuse_counter++;

                if (ReD_PCRT[cpu][PCin_entry].reuse_counter > 1023) {
                    ReD_PCRT[cpu][PCin_entry].reuse_counter >>= 1;
                    ReD_PCRT[cpu][PCin_entry].noreuse_counter >>= 1;
                }

                ReD_ART[cpu][ART_set].adds[i].valid[subsector] = 0;
            }
            return 1;
        }
    }
    return 0;
}

void remember(uint32_t cpu, uint64_t PCnow, uint64_t block) {
    uint32_t where;
    uint64_t i, tag, subsector, PCev_entry, PCnow_entry;
    uint64_t ART_set;

    subsector = block & (RED_SECTOR_SIZE-1);
    ART_set = (block >> RED_SECTOR_SIZE_BITS) & (RED_SETS-1);
    tag = (block >> (RED_SETS_BITS + RED_SECTOR_SIZE_BITS)) & (RED_TAG_SIZE-1);

    PCnow_entry = (PCnow >> 2) & (PCs-1);

    for (i = 0; i < RED_WAYS; i++) {
        if (ReD_ART[cpu][ART_set].adds[i].tag == tag)
            break;
    }

    if (i != RED_WAYS) {
        ReD_ART[cpu][ART_set].adds[i].valid[subsector] = 1;

        if (ART_set % RED_PC_FRACTION == 0) {
            ReD_ART_PC[cpu][ART_set / RED_PC_FRACTION][i].pc_entry[subsector] = PCnow_entry;
        }
    } else {
        where = ReD_ART[cpu][ART_set].insert;

        if (ART_set % RED_PC_FRACTION == 0) {
            for (int s = 0; s < RED_SECTOR_SIZE; s++) {
                if (ReD_ART[cpu][ART_set].adds[where].valid[s]) {
                    PCev_entry = ReD_ART_PC[cpu][ART_set / RED_PC_FRACTION][where].pc_entry[s];
                    ReD_PCRT[cpu][PCev_entry].noreuse_counter++;

                    if (ReD_PCRT[cpu][PCev_entry].noreuse_counter > 1023) {
                        ReD_PCRT[cpu][PCev_entry].reuse_counter >>= 1;
                        ReD_PCRT[cpu][PCev_entry].noreuse_counter >>= 1;
                    }
                }
            }
        }

        ReD_ART[cpu][ART_set].adds[where].tag = tag;
        for (int j = 0; j < RED_SECTOR_SIZE; j++) {
            ReD_ART[cpu][ART_set].adds[where].valid[j] = 0;
        }
        ReD_ART[cpu][ART_set].adds[where].valid[subsector] = 1;

        if (ART_set % RED_PC_FRACTION == 0) {
            ReD_ART_PC[cpu][ART_set / RED_PC_FRACTION][where].pc_entry[subsector] = PCnow_entry;
        }

        ReD_ART[cpu][ART_set].insert++;
        if (ReD_ART[cpu][ART_set].insert == RED_WAYS) 
            ReD_ART[cpu][ART_set].insert = 0;
    }
}