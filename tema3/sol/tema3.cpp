#include <iostream>
#include "tema3.h"

using namespace std;

void M(int src, int dest) {
    cout << "M(" << src << "," << dest <<")\n";
}

void print_topology(int rank, map<int, int*> topology, int size) {
    cout << rank << " -> ";
    for (auto x : topology) {
        cout << x.first << ":";
        for(int c = 0; c < size; c++) {
            cout << x.second[c];
            if( x.second[c + 1] != 0)
                cout << ",";
            else 
                break;
        }
        cout << " ";
    }
    cout << endl;
}

int get_coordinator_for_worker(int worker_rank, map<int, int*> topology, int size) {
    for (auto x : topology) {
        for(int c = 0; c < size; c++) {
            if(x.second[c] == worker_rank) {
                return x.first;
            }
        }
    }
    return -1;
}

int read_data(int rank, int** workers) {
    int counter;
    string filename;
    /* Choose corresponding input file */
    switch (rank) {
        case MASTER_0:
            filename = FILE_0;
            break;
        case MASTER_1:
            filename = FILE_1;
            break;
        case MASTER_2:
            filename = FILE_2;
    }

    fstream input;
    int aux;

    input.open(filename);
    /* Read the number of workers */
    input >> counter;
    /* Read the array of workers */
    for(int i = 0; i < counter; i++) {
        input >> aux;
        /* Let the worker know that this process is its coordinator */
        MPI_Send(&rank, 1, MPI_INT, aux, 0, MPI_COMM_WORLD); 
        M(rank, aux);

        (*workers)[i] = aux;
    }
    input.close();
    return counter;
}

/* Function passed to threads for initializing the array */
void *f(void *arg) {
    T_arg input = *(T_arg *)arg;
    int start = input.id * (double)input.N / THREADS_NO;
    int end = mini((input.id + 1) * (double)input.N / THREADS_NO, input.N);
    int *arr = *input.arr;

    for(int i = start; i < end; i++) {
        arr[i] = i;
    }
    input.arr = &arr;
    pthread_exit(NULL);
}

int create_topology(int rank, int no_mpi_procs, map<int, int*> *topology, int **workers, int* coordinator) {
    int *aux_arr1 = (int*) calloc(no_mpi_procs, sizeof(int));
    MPI_Status status;
    int counter = 0;
    if(rank == MASTER_0 || rank == MASTER_1 || rank == MASTER_2) {
        counter = read_data(rank, workers);
        (*topology).insert({rank, *workers});

        if(rank == MASTER_2) {
            /* Process 2 sends its topology to 1 and 0 */
            MPI_Send((*workers), no_mpi_procs, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(MASTER_2, MASTER_0);

            MPI_Send((*workers), no_mpi_procs, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD);
            M(MASTER_2, MASTER_1);

            /* Process 2 receives 1's and 0's topologies */
            MPI_Recv(aux_arr1, no_mpi_procs, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD, &status);
            (*topology).insert({MASTER_0, aux_arr1});

            int *aux_arr2 = (int*) calloc(no_mpi_procs, sizeof(int));
            MPI_Recv(aux_arr2, no_mpi_procs, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD, &status);
            (*topology).insert({MASTER_1, aux_arr2});

            /* Process 2 sends 1's topology to 0, and 0's topology to 1 */
            MPI_Send(aux_arr1, no_mpi_procs, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD);
            M(MASTER_2, MASTER_1);

            MPI_Send(aux_arr2, no_mpi_procs, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(MASTER_2, MASTER_0);
        } else {
            /* 0 and 1 send their topologies to ROOT */
            MPI_Send((*workers), no_mpi_procs, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD);
            M(rank, MASTER_2);
            /* Receive ROOT's topology */
            MPI_Recv(aux_arr1, no_mpi_procs, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);
            (*topology).insert({MASTER_2, aux_arr1});
            /* Receive the topology of the other process */
            int *aux_arr2 = (int*) calloc(no_mpi_procs, sizeof(int));
            MPI_Recv(aux_arr2, no_mpi_procs, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);
            (*topology).insert({(rank == MASTER_1) ? MASTER_0 : MASTER_1, aux_arr2});
        }
        /* Send the complete topology to all the workers connected to this process */
        for(int i = 0; i < counter; i++) {
            for(auto entry : (*topology)) {
                /* Send the master's ID */
                MPI_Send(&entry.first, 1, MPI_INT, (*workers)[i], 0, MPI_COMM_WORLD);
                M(rank, (*workers)[i]);
                /* Send the IDs of its workers */
                MPI_Send(entry.second, no_mpi_procs, MPI_INT, (*workers)[i], 0, MPI_COMM_WORLD);
                M(rank, (*workers)[i]);
            }
        }
    } else {
        MPI_Status status;
        /* A worker process receives the ID belonging to its coorindator*/
        MPI_Recv(coordinator, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        int src;
        /* Receives 3 topologies, one for each master */
        for(int i = 0; i < 3; i++) {
            MPI_Recv(&src, 1, MPI_INT, *coordinator, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(aux_arr1, no_mpi_procs, MPI_INT, *coordinator, 0, MPI_COMM_WORLD, &status);
            (*topology).insert({src, aux_arr1});
            aux_arr1 = (int*) calloc(no_mpi_procs, sizeof(int));
        }

    }
    /* Display the topology*/
    print_topology(rank, *topology, no_mpi_procs);
    return counter;
}

int main(int argc, char **argv){
    int no_mpi_procs, rank, provided, coordinator;
    MPI_Status status;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &no_mpi_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int *workers = (int*) calloc(no_mpi_procs, sizeof(int));
    // int *aux_arr1 = (int*) calloc(no_mpi_procs, sizeof(int));

    map<int, int*> topology;
    int counter = create_topology(rank, no_mpi_procs, &topology, &workers, &coordinator);

    /* Map with all the workers and their corresponding coordinators*/
    map<int, int> masters;
    for(int i = 3; i < no_mpi_procs; i++) {
        int master = get_coordinator_for_worker(i, topology, no_mpi_procs);
        masters.insert({i, master});
    }

    int size, *arr;
    if(rank == MASTER_0) {
        /* Read the size of the array*/
        int n = atoi(argv[1]);
        arr = (int*)calloc(n, sizeof(int));
        if(arr == nullptr) {
            cout << "Allocation failure." << endl;
            exit(EXIT_FAILURE);
        }

        /* Create the array using threads */
        pthread_t *threads = (pthread_t*)calloc(THREADS_NO + 1, sizeof(pthread_t));
        T_arg *argums = (T_arg*)calloc(THREADS_NO + 1, sizeof(T_arg));
        for(int i = 0; i < THREADS_NO; i++) {
            argums[i].arr = &arr;
            argums[i].N = n;
            argums[i].id = i;
            pthread_create(&threads[i], NULL, f, &argums[i]);
        }

        for(int i = 0; i < THREADS_NO; i++) {
            pthread_join(threads[i], NULL);
        }

        free(threads);
        free(argums);

        int workers_no = no_mpi_procs - 3, start, end;
        /* The number of elements which is received by each worker */
        int chunk_size = ceil((double)n / workers_no);
        for(int i = 3; i < no_mpi_procs; i++) {
            /* Split the array into smaller ones to be passed to workers */
            start = chunk_size * (i - 3);
            end = min((i - 2) * chunk_size, n);
            size = (end - start);

            int *aux = (int*)calloc(size, sizeof (int));
            for(int j = start; j < end; j++) {
                aux[j - start] = arr[j];
            }
            /* If the current worker is corresponding to process 0,
            the data can be sent directly to it */
            if(masters.at(i) == MASTER_0) {
                MPI_Send(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                M(rank, i);

                MPI_Send(aux, size, MPI_INT, i, 0, MPI_COMM_WORLD);
                M(rank, i);
                free(aux);
            } else {
                /* All other messages are sent through process 2.
                The rank of the destination-process is also sent to be able to identify it. */
                MPI_Send(&i, 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD);
                M(rank, MASTER_2);

                MPI_Send(&size, 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD);
                M(rank, MASTER_2);

                MPI_Send(aux, size, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD);
                M(rank, MASTER_2);
                free(aux);
            }
        }
        int worker_rank, size2;
        /* Process 0 receives all the data and reconstructs the array */
        for(int i = 3; i < no_mpi_procs; i++) {
            int master = masters.at(i);
            int *aux;
            /* All the data from 2's and 1's workers comes through 2 */
            if(master == MASTER_2 || master == MASTER_1) {
                MPI_Recv(&worker_rank, 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(&size2, 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);

                aux = (int*)calloc(size2, sizeof (int));
                MPI_Recv(aux, size2, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);
            } else {
                /* If 0 is the coordinator, the data is received from worker 'i' */
                worker_rank = i;
                MPI_Recv(&size2, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);

                aux = (int*)calloc(size2, sizeof (int));
                MPI_Recv(aux, size2, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            }
            /* Calculate the position in the global array, using the rank of the worker */
            start = chunk_size * (worker_rank - 3);
            end = min((worker_rank - 2) * chunk_size, n);
            for(int j = start; j < end; j++) {
                arr[j] = aux[j - start];
            }
            free(aux);
        }

    } else if(rank == MASTER_2) {
        /* Get the number of workers for process 0 */
        int ones = 0, size1;
        for(int i = 0; i < no_mpi_procs; i++) {
            if(topology.at(MASTER_1)[i] == 0) {
                break;
            }
            ones++;
        }

        /* For all the tasks corresponding to 1's or 2's workers */
        for(int i = 0; i < counter + ones; i++) {
            /* Receive destinations, size and the slice of the array */
            int dest;
            MPI_Recv(&dest, 1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&size1, 1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD, &status);
            int *aux = (int*)calloc(size1, sizeof (int));
            MPI_Recv(aux, size1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD, &status);

            if(masters.at(dest) == MASTER_2) {
                /* If the coordinator is process 2, the message can be sent directly */
                MPI_Send(&size1, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
                M(rank, dest);

                MPI_Send(aux, size1, MPI_INT, dest, 0, MPI_COMM_WORLD);
                M(rank, dest);
                free(aux);
            } else if(masters.at(dest) == MASTER_1) {
                /* If the coordinator is process 1, the message is sent to it */
                MPI_Send(&dest, 1, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD);
                M(rank, MASTER_1);

                MPI_Send(&size1, 1, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD);
                M(rank, MASTER_1);

                MPI_Send(aux, size1, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD);
                M(rank, MASTER_1);
                free(aux);
            }
        } 
        /* Process 2 receives all the data coming from its workers and sends it to process 0 */
        for(int i = 0; i < counter; i++) {
            MPI_Recv(&size1, 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD, &status);
            int* aux = (int*)calloc(size1, sizeof (int));
            MPI_Recv(aux, size1, MPI_INT, workers[i], 0, MPI_COMM_WORLD, &status);

            MPI_Send(&workers[i], 1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(rank, MASTER_0);

            MPI_Send(&size1, 1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(rank, MASTER_0);

            MPI_Send(aux, size1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(rank, MASTER_0);
            free(aux);
        }
        /* Process 2 receives all the data coming from process 1 and sends it to process 0 */
        for(int i = 0; i < ones; i++) {
            int size1, worker_rank;
            MPI_Recv(&worker_rank, 1, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&size1, 1, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD, &status);

            int *aux = (int*)calloc(size1, sizeof (int));
            MPI_Recv(aux, size1, MPI_INT, MASTER_1, 0, MPI_COMM_WORLD, &status);

            MPI_Send(&worker_rank, 1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(rank, MASTER_0);

            MPI_Send(&size1, 1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(rank, MASTER_0);

            MPI_Send(aux, size1, MPI_INT, MASTER_0, 0, MPI_COMM_WORLD);
            M(rank, MASTER_0);
            free(aux);
        }
        
    } else if(rank == MASTER_1) {
        /* Process 1 receives tasks from process 2 and sends them to its workers */
        for(int i = 0; i < counter; i++) {
            int dest, size1;
            MPI_Recv(&dest, 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&size1, 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);
            int *aux = (int*)calloc(size1, sizeof (int));
            MPI_Recv(aux, size1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD, &status);

            MPI_Send(&size1, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            M(rank, dest);

            MPI_Send(aux, size1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            M(rank, dest);
            free(aux);
        }
        /* Process 1 receives result from workers and sends it to process 2*/
        for(int i = 0; i < counter; i++) {
            int size1;
            MPI_Recv(&size1, 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD, &status);
            int* aux = (int*)calloc(size1, sizeof (int));
            MPI_Recv(aux, size1, MPI_INT, workers[i], 0, MPI_COMM_WORLD, &status);

            MPI_Send(&workers[i], 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD);
            M(rank, MASTER_2);

            MPI_Send(&size1, 1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD);
            M(rank, MASTER_2);

            MPI_Send(aux, size1, MPI_INT, MASTER_2, 0, MPI_COMM_WORLD);
            M(rank, MASTER_2);
            free(aux);
        }
    } else if(rank > 2) {
        /* If the process is a worker, it receives the array */
        int size1;
        MPI_Recv(&size1, 1, MPI_INT, coordinator, 0, MPI_COMM_WORLD, &status);

        int *aux = (int*)calloc(size1, sizeof (int));
        MPI_Recv(aux, size1, MPI_INT, coordinator, 0, MPI_COMM_WORLD, &status);

        /* Each element is doubled */
        for (int i = 0; i < size1; i++) {
            aux[i] *= 2;
        }
        MPI_Send(&size1, 1, MPI_INT, coordinator, 0, MPI_COMM_WORLD);
        M(rank, coordinator);
        MPI_Send(aux, size1, MPI_INT, coordinator, 0, MPI_COMM_WORLD);
        M(rank, coordinator);
        free(aux);
    }

    /* Display the final array */
    if(rank == MASTER_0) {
        cout << "Rezultat: ";
        for(int i = 0; i < atoi(argv[1]); i++) {
            cout << arr[i] << " ";
        }
        free(arr);
    }
    MPI_Finalize();
}