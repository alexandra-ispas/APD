#ifndef SOL_TEMA3_H
#define SOL_TEMA3_H

#include <cmath>
#include "mpi.h"
#include <fstream>
#include <string.h>
#include <vector>
#include <unistd.h>

#define MASTER_0 0
#define MASTER_1 1
#define MASTER_2 2

#define FILE_0 "cluster0.txt"
#define FILE_1 "cluster1.txt"
#define FILE_2 "cluster2.txt"

#define THREADS_NO  sysconf(_SC_NPROCESSORS_CONF)


#define mini(a, b) a < b ? a : b

/* Arguments passed to a thread */
typedef struct t_arg{
    int id;  // thread id
    int N;   // size of the array
    int **arr;
} T_arg;

#endif //SOL_TEMA3_H
