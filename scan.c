#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#define MAX_LINE_SIZE 256

typedef struct {
    sem_t mutex;
    sem_t barrier;
    int count;
    int max;
} barrier_t;

void barrier_init(barrier_t *barrier, int max) {
    sem_init(&barrier->mutex, 0, 1);
    sem_init(&barrier->barrier, 0, 0);
    barrier->count = 0;
    barrier->max = max;
}

void barrier_wait(barrier_t *barrier) {
    sem_wait(&barrier->mutex);
    barrier->count++;
    sem_post(&barrier->mutex);

    // If this is the last thread to arrive at the barrier, release all threads
    if (barrier->count == barrier->max) {
        for (int i = 0; i < barrier->max; i++) {
            sem_post(&barrier->barrier);
        }
    }

    sem_wait(&barrier->barrier);
    sem_post(&barrier->barrier);
}

typedef struct {
    int *array;
    int array_size;
    int index;
    int chunk;
} thread_args_t;

barrier_t read_barrier;

void *hillis_steele_scan(void *args) {
    thread_args_t *threadArgs = (thread_args_t *) args;
    int *array = threadArgs->array;
    int size = threadArgs->array_size;
    int index = threadArgs->index;
    int chunk = threadArgs->chunk;

    int d = 1;

    while (d < size) {
        int offset = 1 << (d - 1);
        int old_values[size];
        // Ensure in sync read so we dont read updated values
        memcpy(&old_values, array, sizeof(int) * size);
        barrier_wait(&read_barrier);

        //TODO bounds check these things
        for (int i = index; i < threadArgs->index + chunk; i++) {
            if (index + offset < size) {
                array[i + offset] += old_values[i];
            }
        }
        d *= 2;
    }
    return NULL;
}

void read_input_vector(const char *filename, int *array) {
    FILE *fp;
    char *line = malloc(MAX_LINE_SIZE + 1);
    size_t len = MAX_LINE_SIZE;

    //ctrl+d to EOF
    fp = strcmp(filename, "-") ? fopen(filename, "r") : stdin;

    assert(fp != NULL && line != NULL);

    int index = 0;

    while (getline(&line, &len, fp) != -1) {
        array[index] = atoi(line);
        index++;
    }

    free(line);
    fclose(fp);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        puts("Not enough arguments");
        exit(EXIT_FAILURE);
    }

    char *file_name = argv[1];
    int vector_size = strtol(argv[2], NULL, 10);
    int num_threads = strtol(argv[3], NULL, 10);

    int *input_array = malloc(sizeof(int) * vector_size);

    read_input_vector(file_name, input_array); //assign vectors

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    //parallel array for use with the matching thread. keeps track of the threads accessible threads, the range of items
    //to work on, and the array
    thread_args_t *thread_args = malloc(num_threads * sizeof(thread_args_t));

    barrier_init(&read_barrier, num_threads);

    //assumes you wont give it more threads than the size of the array
    int chunk = vector_size / num_threads;

    for (int i = 0; i < num_threads; i++) {
        thread_args[i].array = input_array;
        thread_args[i].array_size = vector_size;
        thread_args[i].index = i * chunk;
        thread_args[i].chunk = chunk;
        pthread_create(&threads[i], NULL, hillis_steele_scan, &thread_args[i]);
    }
    //wait on threads so we dont die before finishing TODO do we even need this?
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    //print out final array
    for (int i = 0; i < vector_size; i++) {
        printf("%d\n", input_array[i]);
    }

    free(input_array);
    free(threads);
    free(thread_args);
}
