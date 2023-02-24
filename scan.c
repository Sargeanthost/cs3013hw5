#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>

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

    // If this is the last thread to arrive at the barrier, release all threads. Mistake in checkpoint was not resetting,
    // so the while loop always had a thread > max, so it would dispatch the thread that just got to the barrier
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
    int ioffset = 0;

    int *old_values = malloc(sizeof(int) * thread_args->array_size);

    //check that our indexes are still being worked on. yield if not?
    for (int offset = 1; offset < thread_args->array_size; offset *= 2) {
        // Ensure in sync read so we dont read updated values
        memcpy(old_values, thread_args->array, sizeof(int) * thread_args->array_size);

        barrier_wait(&read_barrier);

        for (int i = thread_args->index; i < thread_args->index + thread_args->chunk; i++) {
            ioffset = i+offset;
            if (ioffset< thread_args->array_size) {
                thread_args->array[ioffset] += old_values[i];
            }
        }
        barrier_wait(&read_barrier);
    }
    return NULL;
}

void read_input_vector(const char *filename, int *array) {
    FILE *fp;
    char *line = malloc(MAX_LINE_SIZE + 1);
    size_t len = MAX_LINE_SIZE;
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
    char *file_name = argv[1];
    int vector_size = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

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

    // Print out final array
    for (int i = 0; i < vector_size; i++) {
        printf("%d\n", input_array[i]);
    }

    free(input_array);
    free(threads);
    free(thread_args);
}
