#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include <iostream>

void measurements(Cache_2_level L) {
    cout << "===== Measurements =====" << endl;

    L.L1_miss_rate = static_cast<float>(L.L1_read_misses + L.L1_write_misses) / static_cast<float>(L.L1_reads + L.L1_writes);
    cout << "a. L1 reads: " << dec << L.L1_reads << endl;
    cout << "b. L1 read misses: " << L.L1_read_misses << endl;
    cout << "c. L1 writes: " << L.L1_writes << endl;
    cout << "d. L1 write misses: " << L.L1_write_misses << endl;
    cout << "e. L1 miss rate: " << fixed << setprecision(4) << L.L1_miss_rate << endl;
    cout << "f. L1 writebacks: " << L.L1_to_L2_writebacks << endl;
    cout << "g. L1 prefetches: 0" << endl;

    L.L2_miss_rate = static_cast<float>(L.L2_read_misses + L.L2_write_misses) / static_cast<float>(L.L2_reads);
    cout << "h. L2 reads (demand): " << L.L2_reads << endl;
    cout << "i. L2 read misses (demand): " << L.L2_read_misses << endl;
    cout << "j. L2 reads (prefetch): 0" << endl;
    cout << "k. L2 read misses (prefetch): 0" << endl;
    cout << "l. L2 writes: " << L.L2_writes << endl;
    cout << "m. L2 write misses: " << L.L2_write_misses << endl;
    cout << "n. L2 miss rate: " << fixed << setprecision(4) << L.L2_miss_rate << endl;    
    cout << "o. L2 writebacks: " << L.L2_to_main_mem_writebacks << endl;
    cout << "p. L2 prefetches: 0" << endl;
    cout << "q. Memory traffic: " << L.memory_traffic << endl;
}

void L1_measurements(Cache L) {
    cout << "===== Measurements =====" << endl;

    L.miss_rate = static_cast<float>(L.read_misses + L.write_misses) / static_cast<float>(L.reads + L.writes);
    cout << "a. L1 reads: " << dec << L.reads << endl;
    cout << "b. L1 read misses: " << L.read_misses << endl;
    cout << "c. L1 writes: " << L.writes << endl;
    cout << "d. L1 write misses: " << L.write_misses << endl;
    cout << "e. L1 miss rate: " << fixed << setprecision(4) << L.miss_rate << endl;
    cout << "f. L1 writebacks: " << L.writebacks << endl;
    cout << "g. L1 prefetches: 0" << endl;   
    cout << "h. L2 reads (demand): " << 0 << endl;
    cout << "i. L2 read misses (demand): " << 0 << endl;
    cout << "j. L2 reads (prefetch): 0" << endl;
    cout << "k. L2 read misses (prefetch): 0" << endl;
    cout << "l. L2 writes: " << 0 << endl;
    cout << "m. L2 write misses: " << 0 << endl;
    cout << "n. L2 miss rate: " << fixed << setprecision(4) << 0 << endl;    
    cout << "o. L2 writebacks: " << 0 << endl;
    cout << "p. L2 prefetches: 0" << endl;
    cout << "q. Memory traffic: " << L.memory_traffic << endl; 
}

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
int main (int argc, char *argv[]) {
    FILE *fp;			// File pointer.
    char *trace_file;		// This variable holds the trace file name.
    cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
    char rw;			// This variable holds the request's type (read or write) obtained from the trace.
    uint32_t addr;		// This variable holds the request's address obtained from the trace.
                    // The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

    // Exit with an error if the number of command-line arguments is incorrect.
    if (argc != 9) {
        printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
        exit(EXIT_FAILURE);
    }
        
    // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
    params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
    params.L1_SIZE   = (uint32_t) atoi(argv[2]);
    params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
    params.L2_SIZE   = (uint32_t) atoi(argv[4]);
    params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
    params.PREF_N    = (uint32_t) atoi(argv[6]);
    params.PREF_M    = (uint32_t) atoi(argv[7]);
    trace_file       = argv[8];

    // Open the trace file for reading.
    fp = fopen(trace_file, "r");
    if (fp == (FILE *) NULL) {
        // Exit with an error if file open failed.
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
        
    // Print simulator configuration.
    printf("===== Simulator configuration =====\n");
    printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
    printf("L1_SIZE:    %u\n", params.L1_SIZE);
    printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
    printf("L2_SIZE:    %u\n", params.L2_SIZE);
    printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
    printf("PREF_N:     %u\n", params.PREF_N);
    printf("PREF_M:     %u\n", params.PREF_M);
    printf("trace_file: %s\n", trace_file);
    printf("\n");

    if (params.L2_SIZE && params.L2_ASSOC) {
        Cache_2_level main_cache(params.L1_SIZE, params.BLOCKSIZE, params.L1_ASSOC, params.L2_SIZE, params.L2_ASSOC, params.PREF_N, params.PREF_M);
        fstream new_file;
        new_file.open(trace_file, ios::in);     
        // Checking whether the file is open.
        if (new_file.is_open()) { 
            string trace;
            int count = 1;

            // Read data from the file object and put it into a string.
            while (getline(new_file, trace)) { 
                main_cache.trace_parse(trace);
                count ++;
            }
            
            // Close the file object.
            new_file.close(); 
        }
        main_cache.print_cache();
        cout << endl;        
        measurements(main_cache);
    }

    else {
        Cache L1(params.L1_SIZE, params.BLOCKSIZE, params.L1_ASSOC);
        fstream new_file;
        new_file.open(trace_file, ios::in);     
        // Checking whether the file is open.
        if (new_file.is_open()) { 
            string trace;
            int count = 1;

            // Read data from the file object and put it into a string.
            while (getline(new_file, trace)) { 
                L1.trace_parse(trace);
                count ++;
            }
            
            // Close the file object.
            new_file.close(); 
        }
        L1.print_cache();
        cout << endl;
        L1_measurements(L1);
    }
}
