#ifndef SIM_CACHE_H
#define SIM_CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <string>
#include <fstream>
#include <vector>
#include <iomanip>
#define nullptr NULL

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

using namespace std;

struct cache_element {
    bool valid;
    bool dirty;
    int tag;
    int LRU_counter;
};

struct prefetch_buffer {
    bool valid;
    vector<int> buffer;
};

/******************** ONE LEVEL CACHE ***********************/

class Cache {
    public:
        int size = 0;
        int blocksize = 0;
        int assoc = 0;
        int sets = 0;        
        int tag = 0;
        int index_bits = 0;
        int index = 0;        

        int reads = 0;
        int read_misses = 0;
        int writes = 0;
        int write_misses = 0;
        int writebacks = 0;
        int memory_traffic = 0;
        float miss_rate = 0;
        
        unsigned long memory_address = 0;
        unsigned long block_address = 0;

        string trace;

        char operation;

        vector<vector<cache_element> > cache_map;

    Cache(int size, int blocksize, int assoc) {
        this->size = size;
        this->blocksize = blocksize;
        this->assoc = assoc;
        this->sets = size / (blocksize * assoc);
        cache_layout();
    }

    void cache_layout() {
        cache_map = vector<vector<cache_element> >(sets, vector<cache_element>(assoc, {false, false, 0, -1}));
    }

    void print_cache() {
        int max_LRU_L1 = -1;

        for (int i = 0; i < assoc; i++) {                       // Find block with highest LRU to replace it
            if (cache_map[index][i].LRU_counter >  max_LRU_L1) {
                 max_LRU_L1 = cache_map[index][i].LRU_counter;
            }
        }

        cout << "===== L1 contents =====" << endl;
        int set_valid = false;                   // To print only occupied blocks

        for (int i = 0; i < sets; i++) {
            set_valid = false;
            for (int j = 0; j < assoc; j++) {
                if (cache_map[i][j].valid) {                          
                    set_valid = true;
                    break;
                }
            }

            if (set_valid) {      
                cout << "set\t" << dec << i << ": ";      
                for (int j = 0; j <= max_LRU_L1; j++) {
                    for (int k = 0; k < assoc; k++) {
                        if (cache_map[i][k].LRU_counter == j) {
                            // Access and print individual members of the struct
                            if (cache_map[i][k].valid) {
                                cout << "\t" << hex << cache_map[i][k].tag;
                                if (cache_map[i][k].dirty) {
                                    cout << " D";
                                }
                            }
                        }
                    }
                }
                cout << endl;
            }
        }
    }

    void trace_parse(string trace) {
        istringstream iss(trace);
        iss >> operation;
        iss >> std::hex >> memory_address;

        block_address = memory_address >> int(log2(blocksize)); // Generate block address
        tag = block_address >> int(log2(sets)); // Generate tag
        index = block_address % sets;

        if (operation == 'r') {            
            read();
            reads++ ;
        }
        else if (operation == 'w')
        {
            write();
            writes ++;
        }
        
    }

    void LRU_counter_update(int current_loop_value, bool full_set) {
        if (full_set == false) {
            for (int i = 0; i < assoc; i++) {
                if (cache_map[index][i].valid) {
                    cache_map[index][i].LRU_counter ++;
                }
            }
            cache_map[index][current_loop_value].LRU_counter = 0;
        }  
        else {
            for (int i = 0; i < assoc; i++) {
                if (cache_map[index][i].valid && cache_map[index][i].LRU_counter < cache_map[index][current_loop_value].LRU_counter) {
                    cache_map[index][i].LRU_counter ++;
                }
            }
            cache_map[index][current_loop_value].LRU_counter = 0;
        }  
    }

    void write_back () {
    }

    void bring_in_block(int tag) {                  // Performs the action of both evicting victim block and bringing in new block
        memory_traffic ++;
        bool full_set = true;
        int max_LRU = -1;
        for (int i = 0; i < assoc; i++) {
            if (cache_map[index][i].valid == 0) {
                full_set = false;
                break;
            }
        }
        if (full_set == false) {
            for (int i = 0; i < assoc; i++) {
                if (cache_map[index][i].valid == 0) {
                    cache_map[index][i].tag = tag;
                    cache_map[index][i].valid = 1;
                    LRU_counter_update(i, full_set);
                    if (operation == 'w')
                        cache_map[index][i].dirty = 1;
                    break;
                }
            }
        }
        else {
            for (int i = 0; i < assoc; i++) {                       // Find block with highest LRU to replace it
                if (cache_map[index][i].LRU_counter > max_LRU) {
                    max_LRU = cache_map[index][i].LRU_counter;
                }
            }
            for (int i = 0; i < assoc; i++) {                       // Replace said block and perform write back if necessary
                if (cache_map[index][i].LRU_counter == max_LRU) {
                    if (cache_map[index][i].dirty == 1) {
                        write_back();
                        writebacks ++;
                        memory_traffic ++;
                    }

                    cache_map[index][i].tag = tag;

                    LRU_counter_update(i, full_set);

                    if (operation == 'w')
                        cache_map[index][i].dirty = 1;
                    else
                        cache_map[index][i].dirty = 0;
                    break;
                } 
            }
        }
    }

    void check_presence(int tag, int index) {
        bool tagFound = false;  // Initialize a flag to track if the tag is found
        int current_loop_value = -1;
        
        for (int i = 0; i < assoc; i++) {
            if (cache_map[index][i].tag == tag) {
                current_loop_value = i;

                // Update LRU counters in case of a hit
                for (int j = 0; j < assoc; j++) {
                    if (cache_map[index][j].valid && cache_map[index][j].LRU_counter < cache_map[index][current_loop_value].LRU_counter) {
                        cache_map[index][j].LRU_counter++;
                    }
                }

                cache_map[index][current_loop_value].LRU_counter = 0;
                if (operation == 'w') {
                    cache_map[index][i].dirty = 1;
                }
                
                // Set the flag to true indicating tag was found
                tagFound = true;
                return;
            }
        }
        
        // If the tag was not found in the loop, perform "MISS" logic
        if (!tagFound) {
            bring_in_block(tag);

            if (operation == 'r') {
                read_misses ++;
            }
            else if (operation == 'w')
            {
                write_misses ++;
            }        
        }
    }


    void read() {
        check_presence(tag, index);
    }

    void write() {
        check_presence(tag, index);
    }
};

/********************** TWO LEVEL CACHE ***********************************************/

class Cache_2_level {
    public:
        int L1_size = 0;
        int L1_assoc = 0;
        int L1_sets = 0;        
        int L1_tag = 0;
        int L1_index_bits = 0;
        int L1_index = 0;

        int L2_size = 0;
        int L2_assoc = 0;
        int L2_sets = 0;
        int L2_tag = 0;
        int L2_index_bits = 0;
        int L2_index = 0;

        int number_of_buffers = 0;
        int depth_of_buffer = 0;   

        int blocksize = 0;   

        int L1_reads = 0;
        int L2_reads = 0;
        int L1_read_misses = 0;
        int L2_read_misses = 0;
        int L1_writes = 0;
        int L2_writes = 0;
        int L1_write_misses = 0;
        int L2_write_misses = 0;
        int L1_to_L2_writebacks = 0;
        int L2_to_main_mem_writebacks = 0;
        int memory_traffic = 0;
        float L1_miss_rate = 0;
        float L2_miss_rate = 0;
        
        unsigned long memory_address = 0;
        unsigned long block_address = 0;

        string trace;

        char operation;

        vector<vector<cache_element> > cache_map_L1;     
        vector<vector<cache_element> > cache_map_L2;  

    Cache_2_level(int L1_size, int blocksize, int L1_assoc, int L2_size, int L2_assoc, int number_of_buffers, int depth_of_buffer) {
        this->L1_size = L1_size;
        this->blocksize = blocksize;
        this->L1_assoc = L1_assoc;
        this->L1_sets = L1_size / (blocksize * L1_assoc);
        this->L2_size = L2_size;
        this->L2_assoc = L2_assoc;
        this->L2_sets = L2_size / (blocksize * L2_assoc);
        this->number_of_buffers = number_of_buffers;
        this->depth_of_buffer = depth_of_buffer;
        cache_layout();
    }

    void cache_layout() {
        cache_map_L1 = vector<vector<cache_element> >(L1_sets, vector<cache_element>(L1_assoc, {false, false, 0, -1}));
        cache_map_L2 = vector<vector<cache_element> >(L2_sets, vector<cache_element>(L2_assoc, {false, false, 0, -1}));
    }

    void print_cache() {
        int max_LRU_L1 = -1;
        int max_LRU_L2 = -1;

        for (int i = 0; i < L1_assoc; i++) {                       // Find block with highest LRU to replace it
            if (cache_map_L1[L1_index][i].LRU_counter >  max_LRU_L1) {
                 max_LRU_L1 = cache_map_L1[L1_index][i].LRU_counter;
            }
        }

        for (int i = 0; i < L2_assoc; i++) {                       // Find block with highest LRU to replace it
            if (cache_map_L2[L2_index][i].LRU_counter >  max_LRU_L2) {
                 max_LRU_L2 = cache_map_L2[L2_index][i].LRU_counter;
            }
        }

        cout << "===== L1 contents =====" << endl;
        int set_valid = false;                   // To print only occupied blocks

        for (int i = 0; i < L1_sets; i++) {
            set_valid = false;
            for (int j = 0; j < L1_assoc; j++) {
                if (cache_map_L1[i][j].valid) {                          
                    set_valid = true;
                    break;
                }
            }

            if (set_valid) {      
                cout << "set\t" << dec << i << ": ";      
                for (int j = 0; j <= max_LRU_L1; j++) {
                    for (int k = 0; k < L1_assoc; k++) {
                        if (cache_map_L1[i][k].LRU_counter == j) {
                            // Access and print individual members of the struct
                            if (cache_map_L1[i][k].valid) {
                                cout << "\t" << hex << cache_map_L1[i][k].tag;
                                if (cache_map_L1[i][k].dirty) {
                                    cout << " D";
                                }
                            }
                        }
                    }
                }
                cout << endl;
            }
        }
        cout << endl;

        cout << "===== L2 contents =====" << endl;
        set_valid = false;                   // To print only occupied blocks

        for (int i = 0; i < L2_sets; i++) {
            set_valid = false;
            for (int j = 0; j < L2_assoc; j++) {
                if (cache_map_L2[i][j].valid) {                          
                    set_valid = true;
                    break;
                }
            }

            if (set_valid) {      
                cout << "set\t" << dec << i << ": ";      
                for (int j = 0; j <= max_LRU_L2; j++) {
                    for (int k = 0; k < L2_assoc; k++) {
                        if (cache_map_L2[i][k].LRU_counter == j) {
                            // Access and print individual members of the struct
                            if (cache_map_L2[i][k].valid) {
                                cout << "\t" << hex << cache_map_L2[i][k].tag;
                                if (cache_map_L2[i][k].dirty) {
                                    cout << " D";
                                }
                            }
                        }
                    }
                }
                cout << endl;
            }
        } 
    }

    void trace_parse(string trace) {
        istringstream iss(trace);
        iss >> operation;
        iss >> std::hex >> memory_address;

        block_address = memory_address >> int(log2(blocksize)); // Generate block address
        L1_tag = block_address >> int(log2(L1_sets)); // Generate L1_tag
        L1_index = block_address % L1_sets;
        L2_tag = block_address >> int(log2(L2_sets)); // Generate L2_tag
        L2_index = block_address % L2_sets;

        if (operation == 'r') {            
            read();
            L1_reads ++;
        }
        else if (operation == 'w')
        {
            write();
            L1_writes ++;
        }
        
    }

    void LRU_counter_update(int current_index, int current_loop_value, bool full_set, int cache_level) {
        if (cache_level == 1) {
            if (full_set == false) {
                for (int i = 0; i < L1_assoc; i++) {
                    if (cache_map_L1[current_index][i].valid) {
                        cache_map_L1[current_index][i].LRU_counter ++;
                    }
                }
                cache_map_L1[current_index][current_loop_value].LRU_counter = 0;
            }  
            else {
                for (int i = 0; i < L1_assoc; i++) {
                    if (cache_map_L1[current_index][i].valid && cache_map_L1[current_index][i].LRU_counter < cache_map_L1[current_index][current_loop_value].LRU_counter) {
                        cache_map_L1[current_index][i].LRU_counter ++;
                    }
                }
                cache_map_L1[current_index][current_loop_value].LRU_counter = 0;
            }  
        }
        
        if (cache_level == 2) {
            if (full_set == false) {
                for (int i = 0; i < L2_assoc; i++) {
                    if (cache_map_L2[current_index][i].valid) {
                        cache_map_L2[current_index][i].LRU_counter ++;
                    }
                }
                cache_map_L2[current_index][current_loop_value].LRU_counter = 0;
            }  
            else {
                for (int i = 0; i < L2_assoc; i++) {
                    if (cache_map_L2[current_index][i].valid && cache_map_L2[current_index][i].LRU_counter < cache_map_L2[current_index][current_loop_value].LRU_counter) {
                        cache_map_L2[current_index][i].LRU_counter ++;
                    }
                }
                cache_map_L2[current_index][current_loop_value].LRU_counter = 0;
            }  
        }
    }

    void bring_in_block() {                  // Performs the action of both evicting victim block and bringing in new block
        memory_traffic ++;
        bool full_set = true;
        int max_LRU = -1;
        for (int i = 0; i < L2_assoc; i++) {
            if (cache_map_L2[L2_index][i].valid == 0) {
                full_set = false;
                break;
            }
        }
        if (full_set == false) {
            for (int i = 0; i < L2_assoc; i++) {
                if (cache_map_L2[L2_index][i].valid == 0) {
                    cache_map_L2[L2_index][i].tag = L2_tag;
                    cache_map_L2[L2_index][i].valid = 1;
                    LRU_counter_update(L2_index, i, full_set, 2);
                    copy_from_L2(i);
                    break;
                }
            }
        }
        else {
            for (int i = 0; i < L2_assoc; i++) {                       // Find block with highest LRU to replace it
                if (cache_map_L2[L2_index][i].LRU_counter > max_LRU) {
                    max_LRU = cache_map_L2[L2_index][i].LRU_counter;
                }
            }
            for (int i = 0; i < L2_assoc; i++) {                       // Replace said block and perform write back if necessary
                if (cache_map_L2[L2_index][i].LRU_counter == max_LRU) {
                    if (cache_map_L2[L2_index][i].dirty == 1) {
                        cache_map_L2[L2_index][i].dirty = 0;
                        L2_to_main_mem_writebacks ++;
                        memory_traffic ++;
                    }

                    cache_map_L2[L2_index][i].tag = L2_tag;

                    LRU_counter_update(L2_index, i, full_set, 2);
                    copy_from_L2(i);
                    break;
                } 
            }
        }
    }

    void copy_from_L2(int L2_column) {
        int L2_version = 0;
        L2_version = (L1_tag << int(log2(L1_sets))) | L1_index;
        L2_version = L2_version >> int(log2(L1_sets));

        bool full_set = true;
        int max_LRU = -1;
        for (int i = 0; i < L1_assoc; i++) {
            if (cache_map_L1[L1_index][i].valid == 0) {
                full_set = false;
                break;
            }
        }

        if (full_set == false) {
            for (int i = 0; i < L1_assoc; i++) {
                if (cache_map_L1[L1_index][i].valid == 0) {
                    cache_map_L1[L1_index][i].tag = L2_version;
                    cache_map_L1[L1_index][i].valid = 1;
                    LRU_counter_update(L1_index, i, full_set, 1);
                    LRU_counter_update(L2_index, L2_column, full_set, 2);
                    if (operation == 'w')
                        cache_map_L1[L1_index][i].dirty = 1;
                    break;
                }
            }
        }

        else {
            for (int i = 0; i < L1_assoc; i++) {                       // Find block with highest LRU to replace it
                if (cache_map_L1[L1_index][i].LRU_counter > max_LRU) {
                    max_LRU = cache_map_L1[L1_index][i].LRU_counter;
                }
            }
            for (int i = 0; i < L1_assoc; i++) {                       // Replace said block and perform write back if necessary
                if (cache_map_L1[L1_index][i].LRU_counter == max_LRU) {
                    int replaced_block_address;
                    int replaced_block_tag;
                    int replaced_block_index;
                    int replaced_block_column;
                    replaced_block_address = (cache_map_L1[L1_index][i].tag << int(log2(L1_sets))) | L1_index;
                    replaced_block_tag = replaced_block_address >> int(log2(L2_sets));
                    replaced_block_index = replaced_block_address % L2_sets;
                    if (cache_map_L1[L1_index][i].dirty == 1) {
                        for(int j = 0; j < L2_assoc; j ++) {
                            if (cache_map_L2[replaced_block_index][j].tag == replaced_block_tag) {
                                cache_map_L2[replaced_block_index][j].dirty = 1;
                                LRU_counter_update(replaced_block_index, j, full_set, 2);
                                L1_to_L2_writebacks ++;
                                break;
                            }
                        }
                        L2_writes ++;
                    }

                    cache_map_L1[L1_index][i].tag = L2_version;

                    LRU_counter_update(L1_index, i, full_set, 1);
                    LRU_counter_update(L2_index, L2_column, full_set, 2);

                    if (operation == 'w')
                        cache_map_L1[L1_index][i].dirty = 1;
                    else
                        cache_map_L1[L1_index][i].dirty = 0;
                    break;
                } 
            }
        }
    }
    void check_presence(int cache_level) {
        if (cache_level == 1) {
            bool tagFound = false;  // Initialize a flag to track if the L1_tag is found
            int current_loop_value = 0;
            
            for (int i = 0; i < L1_assoc; i++) {
                if (cache_map_L1[L1_index][i].tag == L1_tag) {
                    current_loop_value = i;

                    // Update LRU counters in case of a hit
                    for (int j = 0; j < L1_assoc; j++) {
                        if (cache_map_L1[L1_index][j].valid && cache_map_L1[L1_index][j].LRU_counter < cache_map_L1[L1_index][current_loop_value].LRU_counter) {
                            cache_map_L1[L1_index][j].LRU_counter++;
                        }
                    }

                    cache_map_L1[L1_index][current_loop_value].LRU_counter = 0;

                    if (operation == 'w') {
                        cache_map_L1[L1_index][i].dirty = 1;
                    }
                    
                    // Set the flag to true indicating L1_tag was found
                    tagFound = true;
                    return;
                }
            }
            
            // If the L1_tag was not found in the loop, perform "MISS" logic
            if (!tagFound) {
                if (operation == 'r') {
                    L1_read_misses ++;
                }
                else if (operation == 'w')
                {
                    L1_write_misses ++;
                }        
                
                check_presence(2);
            }
        }

        if (cache_level == 2) {
            L2_reads ++;
            bool tagFound = false;  // Initialize a flag to track if the L1_tag is found
            int current_loop_value;

            bool full_set = true;
            int max_LRU = -1;
            for (int i = 0; i < L2_assoc; i++) {
                if (cache_map_L2[L2_index][i].valid == 0) {
                    full_set = false;
                    break;
                }
            }
            
            for (int i = 0; i < L2_assoc; i++) {
                if (cache_map_L2[L2_index][i].tag == L2_tag) {
                    current_loop_value = i;
                    
                    // Set the flag to true indicating L1_tag was found
                    tagFound = true;

                    // Copy block to L1
                    copy_from_L2(i);
                    return;
                }
            }
            
            // If the L1_tag was not found in the loop, perform "MISS" logic
            if (!tagFound) {
                bring_in_block();
                L2_read_misses ++;      
            }
        }
    }

    void read() {
        check_presence(1);
    }

    void write() {
        check_presence(1);
    }
};

#endif