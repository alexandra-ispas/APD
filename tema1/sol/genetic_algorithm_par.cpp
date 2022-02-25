#include "helper.h"

void DIE(bool condition, string message){
	if(condition) {
		cout << message << endl;
		exit(EXIT_FAILURE);
	}
}

int cmpfunc(individual first, individual second)
{
    int i;
    int res = second.fitness - first.fitness; // decreasing by fitness
    if (res == 0) {
        int first_count = 0, second_count = 0;
        for (i = 0; i < first.chromosome_length && i < second.chromosome_length; ++i) {
            first_count += first.chromosomes[i];
            second_count += second.chromosomes[i];
        }
        res = first_count - second_count; // increasing by number of objects in the sack
        if (res == 0) {
            return second.index - first.index;
        }
    }
    return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void merge(individual **source, int start, int mid, int end, individual **destination) {
	int iA = start;
	int iB = mid;
	int i;

	for (i = start; i < end; i++) {
		if (end == iB || (iA < mid && cmpfunc((*source)[iA], (*source)[iB]) < 0)) {
			(*destination)[i] = (*source)[iA];
			iA++;
		} else {
			(*destination)[i] = (*source)[iB];
			iB++;
		}
	}
}

void *run_genetic_algorithm_par(void *args)
{
    thread_input input = *(thread_input*)args;
    int object_count = input.object_count;
    int thread_id = input.thread_id;
    int sack_capacity = input.sack_capacity;
	sack_object *objects = input.objects;
    int generations_count = input.generations_count;
    int P = input.P, cursor, count;

	individual **current_generation = input.current_generation;
	individual **next_generation = input.next_generation;
	individual *tmp = NULL;

    int start_obj = thread_id * (double)object_count / P;
    int end_obj = min(object_count, (thread_id + 1) * (double)object_count / P);
	int i, j, k, r, weight, profit, width;

	for (i = start_obj; i < end_obj; ++i) {
		(*current_generation)[i].fitness = 0;
		(*current_generation)[i].chromosomes = (int*)calloc(object_count, sizeof(int));
		(*current_generation)[i].chromosomes[i] = 1;
		(*current_generation)[i].index = i;
		(*current_generation)[i].chromosome_length = object_count;

		(*next_generation)[i].fitness = 0;
		(*next_generation)[i].chromosomes = (int*)calloc(object_count, sizeof(int));
		(*next_generation)[i].index = i;
		(*next_generation)[i].chromosome_length = object_count;

		(*input.aux)[i].chromosomes = (int*)calloc(object_count, sizeof(int));
		(*input.aux)[i].fitness = -1;
	}
    r = pthread_barrier_wait(input.barrier);
	DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");

	for (k = 0; k < generations_count; ++k) {
		cursor = 0;

		// se recalculeaza fitness
        for (i = start_obj; i < end_obj; ++i) {
			// se face update la undecsi
			(*current_generation)[i].index = i;
			
            weight = 0;
            profit = 0;
            for (j = 0; j < (*current_generation)[i].chromosome_length; ++j) {
                if ((*current_generation)[i].chromosomes[j]) {
                    weight += objects[j].weight;
                    profit += objects[j].profit;
                }
            }
            (*current_generation)[i].fitness = (weight <= sack_capacity) ? profit : 0;
        }

        r = pthread_barrier_wait(input.barrier);
		DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");

		// mergesort
		for (width = 1; width < input.nr; width = 2 * width) {
			for (i = 2 * thread_id * width; i < input.nr; i = i + 2 * width * P) {
				merge(current_generation, i, i + width, i + 2 * width, input.aux);
			}
			pthread_barrier_wait(input.barrier);
			if (thread_id == 0) {
				tmp = *input.aux;
				*input.aux = *current_generation;
				*current_generation = tmp;
			}
			pthread_barrier_wait(input.barrier);
		}

		// se pastreza primii 30% din copii
		for (i = start_obj * 3 / 10; i < end_obj * 3 / 10; ++i) {
			memcpy((*next_generation)[i].chromosomes, (*current_generation)[i].chromosomes,
                   (*current_generation)[i].chromosome_length * sizeof(int));
		}
		r = pthread_barrier_wait(input.barrier);
		DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");

		
		// mutatiile corespunzatoare pentru primii 40%
		cursor = object_count * 3 / 10;
		count = object_count * 2 / 10;
		for (i = start_obj * 4 / 10; i < end_obj * 4 / 10; ++i) {
			if(i < count){
				// se afla in primii 20%
				memcpy((*next_generation)[cursor + i].chromosomes, (*current_generation)[i].chromosomes,
						(*current_generation)[i].chromosome_length * sizeof(int));
				mutate_bit_string_1((*next_generation) + cursor + i, k);
			} else {
				// se afla in urmatorii 20%
				memcpy((*next_generation)[2 * count + i].chromosomes, (*current_generation)[i + count].chromosomes,
						(*current_generation)[i + count].chromosome_length * sizeof(int));
				mutate_bit_string_2((*next_generation) + 2 * count + i, k);
			}
		}
		r = pthread_barrier_wait(input.barrier);
		DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");

		cursor += 2 * count;

		// crossover pentru primii 30% din parinti
		for(i = start_obj * 3 / 10; i < end_obj * 3 / 10 - 1; i+=2) {
			crossover((*current_generation) + i, (*next_generation) + cursor + i, k);
		}
		r = pthread_barrier_wait(input.barrier);
		DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");

		count = object_count * 3 / 10;
		if(thread_id == 0){
			if (count & 1) {
				// acopera cazul in care, daca numarul de parinit este impar,
            	// ultimul nu se modifica
				memcpy((*next_generation)[cursor + count - 1].chromosomes,
                       (*current_generation)[object_count - 1].chromosomes,
						(*current_generation)[object_count - 1].chromosome_length * sizeof(int));
			}
			// switch-ul intre generatii
			tmp = *current_generation;
			*current_generation = *next_generation;
			*next_generation = tmp;
			if (k % 5 == 0) {
				cout << (*current_generation)[0].fitness << endl;
			}
		
		}
		r = pthread_barrier_wait(input.barrier);
		DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");
	}
	r = pthread_barrier_wait(input.barrier);
	DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");
	
	// se recalculeaza fitness
	for (i = start_obj; i < end_obj; ++i) {
		weight = 0;
		profit = 0;
		for (j = 0; j < (*current_generation)[i].chromosome_length; ++j) {
			if ((*current_generation)[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}
		(*current_generation)[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
	r = pthread_barrier_wait(input.barrier);
	DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");

	// mergesort
	for (width = 1; width < input.nr; width = 2 * width) {
		for (i = 2 * thread_id * width; i < input.nr; i = i + 2 * width * P) {
			merge(current_generation, i, i + width, i + 2 * width, input.aux);
		}
		pthread_barrier_wait(input.barrier);

		if (thread_id == 0) {
			tmp = *input.aux;
			*input.aux = *current_generation;
			*current_generation = tmp;
		}
		pthread_barrier_wait(input.barrier);
	}
	
	if(thread_id == 0){
		cout << (*current_generation)[0].fitness << endl;
	}
	r = pthread_barrier_wait(input.barrier);
	DIE((r && r != PTHREAD_BARRIER_SERIAL_THREAD), "Eroare la bariera.");

	// eliberez memoria pentru generatii
	for(i = start_obj; i < end_obj; i++){
		free((*current_generation)[i].chromosomes);
		free((*next_generation)[i].chromosomes);
	}
    pthread_exit(NULL);
}
