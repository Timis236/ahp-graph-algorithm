#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// Definitions

// Choose Parameters
#define N 6					// Set the number of nodes
#define NUM_THREADS 4		// Set the number of threads to use
#define INC 64				// Set the amount of graphs to check per thread loop
#define MAX_SIZE 32768		// Set the max size of the cnt array

// Choose Pattern to Search For (define the desired pattern as 1)
#define ALT_HAM_PATH 1		// Alternating Hamilton Paths

// Choose Outputs to Produce
#define PATTERN_OUTPUT 1	// 1/0 to choose whether to create a file containing all working graphs/patterns
#define COUNT_OUTPUT 1		// 1/0 to choose whether to output the total graphs with each number of solutions into the console

// Auxiliary Functions
// factorial function
int fact(int n) {
	int total = 1;

	for (int i = n; i > 0; i--) {
		total *= i;
	}

	return total;
}

// swap function
void swap(int* v, const int i, const int j) {
	int t;
	t = v[i];
	v[i] = v[j];
	v[j] = t;
}

// Global Variables
volatile int g_index;
volatile unsigned cnt[MAX_SIZE];
int fin;

// Main Functions

// map matrix function
char** map_matrix(int s) {

	// initialize the graph (allocate memory so that it can be referenced in other functions)
	char **graph = malloc(sizeof(int*)*N);
	for (int i = 0; i < N; i++) {
		graph[i] = malloc(sizeof(int)*N);
		for (int j = 0; j < N; j++) {
			graph[i][j] = 0;
		}
	}

	// create a adjacency/coloring matrix from the graph array based on the bit representation of the integer s
	// this is used later to check graphs corresponding to some integer s with n choose 2 bits (nC2 bits is equal to the number of edges)
	// s is interpreted as a graph where, in order of most to least significant bits, the bits represent the edges between nodes {0,1}, {1,2}, {2,3}, ..., then {0,2}, {1,3}, ..., all the way to {0,N-1} (the gap between the two numbers decrease depending on the significance of the bit)
	// this keeps the structure of the graph consistent with the definition later in the code when adding the hardcoded path to the largest bits of the graph
	int b = 0;
	int gap = N - 2;
	while (gap >= 0) {
		int i = N - 2 - gap;
		for (int j = N - 1; j > gap; j--) {
			if ((1 << b) & s) {
				graph[i][j] = 1;
			} else {
				graph[i][j] = 0;
			}
			if ((1 << b) & s) {
				graph[j][i] = 1;
			} else {
				graph[j][i] = 0;
			}
			i--;
			b++;
		}
		gap--;
	}

	return graph;
}

// checking function, checks for an alternating hamiltonian path
int chk_ahp(char** graph, int* path) {
	int bad = 0;
	int i = 0;
	while (i < N - 2 && !bad) {
		bad = graph[path[i]][path[i + 1]] == graph[path[i + 1]][path[i + 2]];
		i++;
	}

	return !bad;
}

// search function, searches a path for a given pattern and produces output
int search(char** graph, int* path, FILE* f) {

	// ***TESTING CODE, PRINTS PATH***
	/*
	for (int d = 0; d < N; d++) {
		printf("%d ", path[d]);
	}
	printf("\n");
	fflush(stdout);
	*/
	// **********

	// the check
	if (chk_ahp(graph, path)) {
		// output the working path to the file
		if (PATTERN_OUTPUT) {
			for (int d = 0; d < N; d++) {
				fprintf(f, "%d ", path[d]);
			}
			fprintf(f, "\n");
		}
		// return values are used for the counts
		return 1;
	}

	return 0;
}

// permutation function, uses iterative Heap's Algorithm for generating permutations
int permutations(char** graph, int* path, FILE* f) {
	int i = 1; // Heap's Algorithm index
	int ss[N] = {0}; // Stack state encoding
	int num_appearances = search(graph, path, f); // initialize num_appearances by checking the first permutation

	// generate the rest of the permutations by following the algorithm directly
	// the stack state array is used as a faster substitute for recursion
	while (i < N) {
		if (ss[i] < i) {
			if (i % 2 == 0) {
				swap(path, 0, i);
			} else {
				swap(path, ss[i], i);
			}

			// use the permutation as long as it is not a reverse
			if (path[0] <= path[N-1]) {
				num_appearances += search(graph, path, f);
			}

			ss[i]++;
			i = 1;
		} else {
			ss[i] = 0;
			i++;
		}
	}

	return num_appearances;
}

void* mainThread(void* thread_id) {

	int id = (long long) thread_id;
	// initialize graph related variables
	char** graph;
	int path[N];
	int num_appearances;
	// loop variables
	int c;
	int max;
	int inc = INC;
	// file variables
	char file_name[16];
	FILE* f;
	if (PATTERN_OUTPUT) {
		// initialize thread-specific file name and pointer
		sprintf(file_name, "T%d.txt", id);
		f = fopen(file_name, "w+");
	}

	// main loop, iterate through all possible unique colourings of a graph with N nodes
	do {
		c = __atomic_fetch_add(&g_index, inc, __ATOMIC_RELAXED);
		max = c + inc;
		if (max > fin) {
			max = fin;
		}
		// ***TESTING CODE, PRINTS LOOP/GRAPH VARIABLES***
		/*
		printf("%X\n%X\n%X\n%X\n%X\n\n", g_index, c, max, fin, inc);
		*/
		// **********
		while (c < max) {
			// create a graph from the integer c
			graph = map_matrix(c);

			// ***TESTING CODE, PRINTS GRAPH MATRIX***
			/*
			for (int d = 0; d < N; d++) {
				for (int e = 0; e < N; e++) {
					printf("%d ", graph[d][e]);
				}
				printf("\n");
			}
			printf("\n");
			fflush(stdout);
			*/
			// **********

			// output the graph to the file
			// if a particular graph does not appear in the file, it has no solutions
			if (PATTERN_OUTPUT) {
				for (int a = 0; a < N; a++) {
					for (int b = 0; b < N; b++) {
						fprintf(f, "%d ", graph[a][b]);
					}
					fprintf(f, "\n");
				}
				fprintf(f, "\n");
			}

			// initialize the intermediate permutation list
			for (int j = 0; j < N; j++) {
				path[j] = j;
			}

			// check all permutations of the path array
			num_appearances = permutations(graph, path, f);

			if (COUNT_OUTPUT) {
				// increment the corresponding index in cnt by 2 (since the opposite colouring also has the same solutions) and reset num_appearances
				__atomic_fetch_add(&cnt[num_appearances], 2, __ATOMIC_RELAXED);
				// let index 0 represent the total solutions found for all graphs
				__atomic_fetch_add(&cnt[0], num_appearances, __ATOMIC_RELAXED);
			}

			if (PATTERN_OUTPUT) {
				// add a new line to the file as a separator and print it to the file
				fprintf(f, "\n");
			}

			// free the allocated space for the graph matrix once it has been used
			free(graph);

			// ***TESTING CODE, PRINTS CNT***
			/*
			for (int d = 0; d < fact(N) >> 1; d++) {
				printf("%d ", cnt[d]);
			}
			printf("\n \n \n");
			fflush(stdout);
			*/
			// **********

			c++;
		}

	} while (max < fin);

	if (PATTERN_OUTPUT) {
		// close file
		fclose(f);
	}

	return NULL;
}

// main loop / code
int main() {
	// set begin time (for timing)
	clock_t begin = clock();

	// set shortcut variables
	int num_edges = (N * (N - 1)) >> 1;					// proven fact for a complete graph (/2 replaced by >>1 for efficiency);
	int num_hamiltonian_paths = fact(N) >> 1;			// proven fact for a complete graph (/2 replaced by >>1 for efficiency), this is just used as a soft upper limit for the cnt array to keep it non-arbitrary

	// initialize threads
	pthread_t tid[NUM_THREADS];

	// used in MainThread to define the start point and (implicitly) the end point for each thread
	int start;

	// define pattern to search for
	// we hardcode the pattern into the graph from the start, to ensure that all graphs checked have at least one solution
	// we don't have to check the cases where we hardcode the pattern in every other permutation of nodes since they will all just be isomorphic to the first case
	if (ALT_HAM_PATH) {
		// since an inverted colouring is functionally identical to the original, we only have to hardcode one of the two colourings of an AHP to get all possibilities
		g_index = 0;
		for (int i = 0; i < N; i++) {
			g_index = (g_index << 1) + (i & 1);
		}
		// create the remaining bits corresponding to the remaining edges in the graph that are not a part of the main pattern
		g_index = g_index << (num_edges - N + 1);
	}
	fin = g_index + (1 << (num_edges - N + 1));

	if (COUNT_OUTPUT) {
		// initialize the count array
		for (int k = 0; k < num_hamiltonian_paths; k++) {
			cnt[k] = 0;
		}
	}

	// split the work across all available threads
	for (int i = 1; i < NUM_THREADS; i++) {
		pthread_create(&tid[i], NULL, mainThread, (void*) (long long) i);
	}
	mainThread(0);
	// wait for all threads to finish
	for (int i = NUM_THREADS - 1; i >= 0; i--) {
		pthread_join(tid[i], NULL);
	}

	if (COUNT_OUTPUT) {
		// print the counts and free the array
		for (int k = 0; k < num_hamiltonian_paths; k++) {
			printf("%d ", cnt[k]);
		}
		printf("\n");
	}

	// set end time and output execution time (for timing)
	clock_t end = clock();
	printf("%f", (double) (end - begin) / CLOCKS_PER_SEC);

	// end program
	return 42;
}
