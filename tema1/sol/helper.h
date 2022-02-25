#ifndef TEMA1_HELPER_H
#define TEMA1_HELPER_H

#include "../skel/individual.h"
#include "../skel/sack_object.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <math.h>
#include <string.h>

using namespace std;

#define min(a, b) (a < b ? a : b)
#define ID_SIZE sizeof(individual)


typedef struct _thread_input
{
	sack_object *objects;				// obiectele citite
	int object_count;					// numarul de obiecte
	int generations_count;				// numarul de generatii
	int sack_capacity;					// capacitatea rucsacului
	int thread_id;						// numarul thread-ului
	int P;								// numarul total de thread-uri
	int nr;								// urmatoarea putere a lui 2, mai mare decat object_count
	pthread_barrier_t *barrier;			// bariera
	individual **current_generation;	
	individual **next_generation;
	individual **aux;					// generatie auxiliara, folosita de mergesort
} thread_input;

// functia pe care o primesc thread-urile
void *run_genetic_algorithm_par(void *args);

// compara doi indivizi
int cmpfunc(individual first, individual second);

// functia din laborator
void merge(individual **source, int start, int mid, int end, individual **destination);

// functiile din skelet
void mutate_bit_string_1(const individual *ind, int generation_index);
void mutate_bit_string_2(const individual *ind, int generation_index);
void crossover(individual *parent1, individual *child1, int generation_index);

void DIE(bool condition, string message);
#endif //TEMA1_HELPER_H
