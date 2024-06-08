/*************************
	Date: 11/24/2023
	Class: CS4541
	Assignment: Memory Allocation Lab
	Author(s): Ramses Larabel
**************************/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct Data {
	int new_block;
	int old_block;
	int size;
	char op_field;
};

int root = 0; 						// Global variable to store the first block of the list

struct Data* parseFile(const char*, int*);
int myalloc(uint32_t*, int, bool, bool);
int myrealloc(uint32_t*, int, int, bool, bool);
void myfree(uint32_t*, int, bool);
// void printVerbose(char, uint64_t, int, int, int);

int main(int argc, char* argv[]) {
	char filename[150] = "/0", output_path[150] = "/0";
	bool list_type = false;				// false for implicit list and true for explicit list
	bool fit_type = false;				// false for First fit and Best fit allocation
        int i, n, var_count = 0;
        bool v = false, arg_error = false;
	// Input validation
        for  (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--free-list=implicit") == 0) {
                        list_type = false;
			var_count++;
                } else if (strcmp(argv[i], "--free-list=explicit") == 0) {
                        list_type = true;
                	var_count++;
		} else if (strcmp(argv[i], "--fit=first") == 0) {
			fit_type = false;
			var_count++;
		} else if (strcmp(argv[i], "--fit=best") == 0) {
			fit_type = true;
			var_count++;
                } else {
			if (i == 1) {
                		if (strcmp(argv[1], "-v") == 0) v = true;
                        	else if (strcmp(argv[1], "-o") == 0) strcpy(output_path, argv[2]);
                	} else if (i == 2) {
				if (strcmp(argv[2], "-o") == 0) strcpy(output_path, argv[3]);
                	} else if (i == argc - 1) {
				strcpy(filename, argv[argc - 1]);
				var_count++;
			} else arg_error = true;
		}
        }

        if (arg_error || (var_count != 3)) {
                printf("Bad file name\n");
                printf("usage: ./mysim: [-v] [-o <output-path>] --free-list={implicit or explicit} --fit={first or best} <input file>\n");
                printf("-v: Optional verbose flag that displays trace info\n");
        } else {
		struct Data* data = parseFile(filename, &n);
		uint32_t* heap = malloc(1000 * sizeof(uint32_t));		// Simulating the heap and will contain headers, payloads, and footers
		heap[0] = 0x00000001;						// Placeholder
		heap[1] = 0x00000F98;						// Intial header value
		heap[998] = 0x00000F98;						// Inital footer value
		heap[999] = 0x00000001;						// Placeholder
		for (i = 2; i < 998; i++) heap[i]= 0x000000000;			// Inital Payload
		int* block_ref =  malloc(1000 * sizeof(int));			// This array will keep track of block ref and the pointer values to the heap
		for (i = 0; i < 1000; ++i) block_ref[i] = 0;

		printf("v: %d, o: %s, list: %d, fit: %d, file: %s\n", v, output_path, list_type, fit_type, filename);
		// Going through each line in the input file
		for (i = 0; i < n; i++) {
			if(data[i].op_field == 'a') {
				printf("a, size: %d, new_block: %d\n", data[i].size, data[i].new_block);
				block_ref[data[i].new_block] = myalloc(heap, data[i].size, list_type, fit_type);
			}
			else if(data[i].op_field == 'r') {
				printf("r, size: %d, old_block: %d, new_block: %d\n", data[i].size, data[i].old_block, data[i].new_block);
				block_ref[data[i].new_block] = myrealloc(heap, data[i].size, block_ref[data[i].old_block], list_type, fit_type);
				block_ref[data[i].old_block] = 0;
			}
			else if(data[i].op_field == 'f') {
				printf("f, old_block: %d\n", data[i].old_block);
				myfree(heap, block_ref[data[i].old_block], list_type);
				block_ref[data[i].old_block] = 0;
			}
		}

		for(i = 0; i < 1000; i++) printf("%d, %x\n", i, heap[i]);

		free(data);
		free(heap);
		free(block_ref);
	}
	return 0;
}

struct Data* parseFile(const char* filename, int* numRecords) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to open the file: %s\n", filename);
        exit(1);
    }

    // Define an initial array size 
    int capacity = 10;
    struct Data* data = (struct Data*)malloc(capacity * sizeof(struct Data));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation error.\n");
        exit(1);
    }

    int count = 0;
    char line[256]; // Assuming a maximum line length of 255 characters

    while (fgets(line, sizeof(line), file)) {
        // Parse the line based on the format
        int size = 0, old_block = -1, new_block = -1;
        char op_field;

        if (sscanf(line, "%c, %d, %d, %d\n", &op_field, &size, &old_block, &new_block) == 4 ||
	    sscanf(line, "%c, %d, %d\n", &op_field, &size, &new_block) == 3 ||
	    sscanf(line, "%c, %d\n", &op_field, &old_block) == 2) {
            // Check if the array needs to be resized
            if (count >= capacity) {
                capacity = count + 10;
                data = (struct Data*)realloc(data, capacity * sizeof(struct Data));
                if (data == NULL) {
                    fprintf(stderr, "Memory allocation error.\n");
                    exit(1);
                }
            }

            // Store the parsed data in the struct array
            data[count].op_field = op_field;
            data[count].new_block = new_block;
            data[count].size = size;
	    data[count].old_block = old_block;

            count++;
        } else {
            fprintf(stderr, "Invalid line in the file: %s", line);
        }
    }

    fclose(file);

    // Update the number of records parsed and return the array
    *numRecords = count;
    return data;
}

// Input: simulated heap, size of new block
// Output: returns a pointer value to be stored
int myalloc(uint32_t* heap, int size, bool list_type, bool fit_type) {
	// Fit type = first, list_type = implicit
	bool done_flag = false;
	int header_i = 1, best_header = 1;
	uint32_t size_rounded = 0, current_header = 0;
	uint32_t free_block = 0;
	uint32_t block_size = 0;
	int best_fit = 99999999;
	if (!fit_type ) {
		while(!done_flag) {
			current_header = heap[header_i];
			free_block = current_header & 0x00000001;              // Only need the first bit
         		block_size = current_header >> 3;                      // Logical shit right 3 bits
			// Check if block is free
			if ((free_block == 0) && (size > 0)) {
				// Check if the block has enough room
				size_rounded = ((size + 7) & 0xFFFFFFF8) >> 2;
				if(block_size >= size_rounded) {
					if (fit_type && (best_fit > size_rounded)) {
						best_fit = size_rounded
						best_header = header_i
					}
					if (header_i > 998 || !fit_type) {
						if (fit_type) header_i = best_header;
						// Update header and footer in right position
						heap[header_i] = ((size_rounded + 2) << 2 ) | 0x00000001;
						heap[header_i + size_rounded + 1] = ((size_rounded + 2) << 2 ) | 0x00000001;
						done_flag = true;
						// Update next header and footer
						heap[header_i + size_rounded + 2] = (((block_size << 1) - (size_rounded + 2)) << 2);
                                        	heap[header_i + (block_size << 1) - 1] = (((block_size << 1) - (size_rounded + 2)) << 2);
						if (list_type) {
                       					heap[heap[header_i + 1] + 1] = heap[heap[header_i + 2] + 1];
                                			heap[heap[header_i + 2] + 2] = heap[heap[header_i + 1] + 2];
                        			}
					}
				}
			}
			// move to next header
			if (!done_flag && (free_block != 0 || !list_type)) header_i += (block_size << 1);
                        else if(!done_flag && (free_block == 0) && list_type) header_i = heap[2 + header_i];
			// Set the done flag if out of range
		}
	}
	// Returns header_i that "points" to the desired header block in the heap
	return header_i;
}
// Input: Heap, size of new block, "ptr" to old block, list type, and fit type
// Output: Returns a pointer value to be stored
// Process: Calls myalloc and myfree for user
int myrealloc(uint32_t* heap, int size, int ptr, bool list_type, bool fit_type) {
	int result = 0;
	if (size > 0) {
		result = myalloc(heap, size, list_type, fit_type);
		// Copy payload
                uint32_t copy_size = heap[result] >> 3;
		if (heap[result] > heap[ptr]) copy_size = heap[ptr] >> 3;
		for (int i = 1;i < (copy_size << 1) - 1; ++i) heap[result + i] = heap[ptr + i];
	}

	// Call myfree on old block
	myfree(heap, ptr, list_type);

	return result;
}

// Input: The heap, the "ptr" to the heap, and list type (false = implicit true = explicit)
// Output: None
// Process: Check block near it to see if they are free. If they are coalescing them, if not simply change the header's and footer's first bit to 0
void myfree(uint32_t* heap, int ptr, bool list_type) {
	if (ptr != 0) {
		uint32_t block_size = heap[ptr] >> 3;
		uint32_t next_header = heap[ptr + (block_size << 1)];
		uint32_t last_footer = heap[ptr - 1];
		uint32_t new_bs;
		int new_ptr = ptr;

		if (((next_header & 0x00000001) == 0) && ((last_footer & 0x00000001) == 0)) {
			new_ptr = ptr - (last_footer >> 2);
			if (list_type) {
                                heap[heap[new_ptr + 1] + 1] = heap[heap[new_ptr + 2] + 1];
                                heap[heap[new_ptr + 2] + 2] = heap[heap[new_ptr + 1] + 2];
                                heap[heap[ptr + (block_size << 1) + 1] + 1] = heap[heap[ptr + (block_size << 1) + 2] + 1];
				heap[heap[ptr + (block_size << 1) + 2] + 2] = heap[heap[ptr + (block_size << 1) + 1] + 2];
                        }
			// coalescing with previous and next block
			new_bs = block_size + (next_header >> 3) + (last_footer >> 3);
                        heap[new_ptr] = new_bs << 3;           						// previous block's header
			heap[ptr + (block_size << 1) + (next_header >> 2) - 1] = new_bs << 3;		// Next block's footer
		} else if ((next_header & 0x00000001) == 0) {
			if (list_type) {
                                heap[heap[ptr + (block_size << 1) + 1] + 1] = heap[heap[ptr + (block_size << 1) + 2] + 1];
                                heap[heap[ptr + (block_size << 1) + 2] + 2] = heap[heap[ptr + (block_size << 1) + 1] + 2];
                        }
			// coalescing with next block
			new_bs = block_size + (next_header >> 3);
			heap[ptr] = new_bs << 3;				// Current block's header
			heap[ptr + (new_bs << 1) - 1] = new_bs << 3;		// Next block's footer
		} else if ((last_footer & 0x00000001) == 0) {
			new_ptr = ptr - (last_footer >> 2);
			if (list_type) {
                                heap[heap[new_ptr + 1] + 1] = heap[heap[new_ptr + 2] + 1];
                                heap[heap[new_ptr + 2] + 2] = heap[heap[new_ptr + 1] + 2];
                        }
			// coalescing with previous block
                        new_bs = block_size + (last_footer >> 3);
                        heap[new_ptr] = new_bs << 3;				// Previous block's header
                        heap[ptr + (block_size << 1) - 1] = new_bs << 3;	// Current block's footer
		} else {
			heap[ptr] &= 0xFFFFFFFE;				// Current block's header
			heap[ptr + (block_size << 1) - 1] &= 0xFFFFFFFE;	// Current block's footer
		}

		if (list_type) {
    			// Set the previous and next pointers for the new block
    			heap[new_ptr + 1] = 0;
    			heap[new_ptr + 2] = root;

	    		// Update the previous root previous block
    			if (root != 0) heap[root + 1] = new_ptr;
    			// Update the root
    			root = new_ptr;
		}
	}
}


/*
void printVerbose(char op_field, uint64_t op_address, int op_size, int loadResult, int storeResult) {
	char result1[25], result2[25];
	if (loadResult == 0) strcpy(result1, "hit");
	else if (loadResult == 1) strcpy(result1, "miss");
	else if (loadResult == 2) strcpy(result1, "eviction miss");
	if (storeResult == 0) strcpy(result2, "hit");
        else if (storeResult == 1) strcpy(result2, "miss");
        else if (storeResult == 2) strcpy(result2, "eviction miss");

	if (op_field == 'L') printf("L %lx,%d %s\n", op_address, op_size, result1);
	else if (op_field == 'S') printf("S %lx,%d %s\n", op_address, op_size, result2);
	else if (op_field == 'M') printf("M %lx,%d %s %s\n", op_address, op_size, result1, result2);
	printf("\n");
}
*/
