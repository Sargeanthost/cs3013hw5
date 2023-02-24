#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#define MAX_LINE_SIZE 256

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int max;
} barrier_t;

void barrier_init(barrier_t *barrier, int max) {
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->cond, NULL);
    barrier->count = 0;
    barrier->max = max;
}

void barrier_wait(barrier_t *barrier) {
    pthread_mutex_lock(&barrier->mutex);
    barrier->count++;

    // If this is the last thread to arrive at the barrier, release all threads
    if (barrier->count == barrier->max) {
        barrier->count = 0;
        pthread_cond_broadcast(&barrier->cond);
    } else {
        // Wait for the last thread to arrive
        pthread_cond_wait(&barrier->cond, &barrier->mutex);
    }

    pthread_mutex_unlock(&barrier->mutex);
}

typedef struct {
    int *array;
    int array_size;
    int index;
    int chunk;
} thread_args_t;

barrier_t read_barrier;

void *hillis_steele_scan(void *args) {
    thread_args_t *thread_args = (thread_args_t *) args;
    int *array = thread_args->array;
    int size = thread_args->array_size;
    int index = thread_args->index;
    int chunk = thread_args->chunk;

    //rounds
    int temp_writes[size];
    for (int displace = 1; displace < size; displace *= 2) {
        printf("ROUND %d\n", displace);
        memcpy(&temp_writes, array, sizeof(int) * size);
        for (int i = index; i < thread_args->index + chunk; i++) {
            if (i + displace < size) {
                printf("Index %d of temp_writes is being written to by thread %ld with value %d\n", (i + displace),
                       syscall(__NR_gettid), (array[i + displace] + array[i]));
                temp_writes[i + displace] = array[i + displace] + array[i];
            }
        }
        barrier_wait(&read_barrier);

        for (int i = index; i < thread_args->index + chunk; i++) {
            printf("Index %d of array is being written to by thread %ld with value %d\n", i,
                   syscall(__NR_gettid), temp_writes[i]);
            array[i] = temp_writes[i];
        }
        barrier_wait(&read_barrier);

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
