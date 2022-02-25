#include "helper.h"

int main(int argc, char *argv[]) {

    int r = 0, i;
    int object_count = 0;
    int sack_capacity = 0;
    int generations_count = (int) strtol(argv[2], NULL, 10);
    int P = (int) strtol(argv[3], NULL, 10);;
    sack_object *objects = NULL;

    fstream fp;
    fp.open(argv[1]);
    if(!fp) {
        cerr << "Could not open file." << endl;
        exit(EXIT_FAILURE);
    }
    fp >> object_count;
    fp >> sack_capacity;

    double nr = log2(object_count);
    if(ceil(nr) != floor(nr)) {
        nr = pow(2 , (int) (nr + 1));
    } else {
        nr = object_count;
    }
    objects = (sack_object *) calloc(object_count, sizeof(sack_object));
    individual *current_generation = (individual*)calloc(nr, ID_SIZE);
	individual *next_generation = (individual*)calloc(nr, ID_SIZE);
    individual *aux = (individual*)calloc(nr, ID_SIZE);
	
    for (i = 0; i < object_count; ++i) {
        fp >> objects[i].profit;
        fp >> objects[i].weight;
    }
    fp.close();

    pthread_barrier_t barrier;
    r = pthread_barrier_init(&barrier, NULL, P);
    DIE(r, "Eroare la crearea barierei.");

    void *status;
	pthread_t threads[P];

    thread_input *arguments = (thread_input*)malloc(P * sizeof(thread_input));
    DIE(arguments == NULL, "Allocation error.");
    
    for (i = 0; i < P; i++) {
		arguments[i].thread_id = i;
        arguments[i].nr = nr;
        arguments[i].sack_capacity = sack_capacity;
        arguments[i].objects = objects;
        arguments[i].object_count = object_count;
        arguments[i].generations_count = generations_count;
        arguments[i].P = P;
        arguments[i].barrier = &barrier;
        arguments[i].current_generation = &current_generation;
        arguments[i].next_generation = &next_generation;
        arguments[i].aux = &aux;
        r = pthread_create(&threads[i], NULL, run_genetic_algorithm_par, &arguments[i]);
		DIE(r, "Eroare la crearea thread-ului");
	}
	for (i = 0; i < P; i++) {
		r = pthread_join(threads[i], &status);
		DIE(r, "Eroare la asteptarea thread-ului");
	}
    r = pthread_barrier_destroy(&barrier);
    DIE(r, "Eroare la distrugerea barierei.");

    free(objects);
    free(arguments);
    free(current_generation);
	free(next_generation);
    return 0;
}
