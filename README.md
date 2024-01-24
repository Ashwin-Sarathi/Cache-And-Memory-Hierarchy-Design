# cache_and_memory_hierarchy_design
lmplentation of a flexible cache and memory hierarchy simulator and used it to compare the performance, area, and energy of different memory hierarchy configurations.

This project was a part of course ECE 563 under Dr. Eric Rotenburg at NC State University.

This project implements an L1 and L2 cache with an LRU replacement policy and WBWA writeback policy to simulate the instructions. 
NOTE: Though the specs go over the use of a prefetch buffer, this implementation does not use a prefetch buffer.

Instructions to run the program:
1. Type "make" to build.  (Type "make clean" first if you already compiled and want to recompile from scratch.)

2. Run trace reader:

   To run without throttling output:
   ./sim 32 8192 4 262144 8 3 10 ../example_trace.txt

   To run with throttling (via "less"):
   ./sim 32 8192 4 262144 8 3 10 ../example_trace.txt | less

   To run and confirm that all requests in the trace were read correctly:
   ./sim 32 8192 4 262144 8 3 10 ../example_trace.txt > echo_trace.txt
   diff ../example_trace.txt echo_trace.txt
	The result of "diff" should indicate that the only difference is that echo_trace.txt has the configuration information.
	0a1,10
	> ===== Simulator configuration =====
	> BLOCKSIZE:  32
	> L1_SIZE:    8192
	> L1_ASSOC:   4
	> L2_SIZE:    262144
	> L2_ASSOC:   8
	> PREF_N:     3
	> PREF_M:     10
	> trace_file: ../example_trace.txt
	> ===================================


 Also in the repository is the project report which gives further insight into the results achieved by the simulator.
