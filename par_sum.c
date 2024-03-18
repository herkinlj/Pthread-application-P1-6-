/*
 * par_sum.c
 *
 * 
 *
 * Compile with --std=c99
 * Authors : Liam J Herkins & Jared Householder
 */


#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Define a structure to represent a task
typedef struct {
    char action;
    long num;
} Task;

// Define a structure to represent the task queue
typedef struct {
    Task* tasks;
    size_t size;
    size_t capacity;
    size_t front;
    size_t rear;
    bool done;
} TaskQueue;

// Global aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

// Function prototypes
void update(long number);
void* worker_function(void* arg);
void enqueue(TaskQueue* queue, Task task);
Task dequeue(TaskQueue* queue);

// Initialize the task queue
void initTaskQueue(TaskQueue* queue, size_t capacity) {
    queue->tasks = (Task*)malloc(capacity * sizeof(Task));
    queue->size = 0;
    queue->capacity = capacity;
    queue->front = 0;
    queue->rear = 0;
    queue->done = false;
}

// Destroy the task queue
void destroyTaskQueue(TaskQueue* queue) {
    free(queue->tasks);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);
}

// Update global aggregate variables given a number
void update(long number) {
    // Simulate computation
    sleep(number);

    // Update aggregate variables
    pthread_mutex_lock(&mutex);
    sum += number;
    if (number % 2 == 1) {
        odd++;
    }
    if (number < min) {
        min = number;
    }
    if (number > max) {
        max = number;
    }
    pthread_mutex_unlock(&mutex);
}

// Worker thread function
void* worker_function(void* arg) {
    TaskQueue* queue = (TaskQueue*)arg;
    while (1) {
        pthread_mutex_lock(&mutex);

        // Wait until the queue is not empty or done
        while (queue->size == 0 && !queue->done) {
            pthread_cond_wait(&not_empty, &mutex);
        }

        // If the queue is empty and done, exit the thread
        if (queue->size == 0 && queue->done) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Dequeue a task from the queue
        Task task = dequeue(queue);
        pthread_mutex_unlock(&mutex);

        // Process the task
        if (task.action == 'p') {
            update(task.num);
        } else if (task.action == 'w') {
            sleep(task.num);
        }
    }
    return NULL;
}

// Enqueue a task into the task queue
void enqueue(TaskQueue* queue, Task task) {
    // Wait until the queue is not full
    while (queue->size == queue->capacity) {
        pthread_cond_wait(&not_full, &mutex);
    }

    // Enqueue the task
    queue->tasks[queue->rear] = task;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;

    // Signal that the queue is not empty
    pthread_cond_signal(&not_empty);
}

// Dequeue a task from the task queue
Task dequeue(TaskQueue* queue) {
    //  Wait until the queue is not empty
    while (queue->size == 0) {
        pthread_cond_wait(&not_empty, &mutex);
    }

    // Dequeue the task
    Task task = queue->tasks[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    // Signal that the queue is not full
    pthread_cond_signal(&not_full);

    return task;
}

int main(int argc, char* argv[]) {
    // Check and parse command line options
    if (argc != 3) {
        printf("Usage: %s <infile> <num_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char* fn = argv[1];
    int num_threads = atoi(argv[2]);

    // Check if the number of threads is valid
    if (num_threads <= 0) {
        printf("ERROR: Invalid number of threads\n");
        exit(EXIT_FAILURE);
    }

    // Open input file
    FILE* fin = fopen(fn, "r");
    if (!fin) {
        printf("ERROR: Could not open %s\n", fn);
        exit(EXIT_FAILURE);
    }

    // Initialize task queue
    TaskQueue queue;
    initTaskQueue(&queue, 100); // Adjust the capacity as needed

    // Create worker threads
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_function, &queue);
    }

    // Load numbers and add them to the queue
    char action;
    long num;
    while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
        // Check for invalid action parameters
        if (num < 1) {
            printf("ERROR: Invalid action parameter: %ld\n", num);
            exit(EXIT_FAILURE);
        }

        // Create a task and enqueue it
        Task task;
        task.action = action;
        task.num = num;
        pthread_mutex_lock(&mutex);
        enqueue(&queue, task);
        pthread_mutex_unlock(&mutex);
    }
    fclose(fin);

    // Mark that no more tasks will be added to the queue
    queue.done = true;
    pthread_mutex_lock(&mutex);
    // Wake up any waiting threads
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&mutex);

    // Wait for worker threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destroy the task queue
    destroyTaskQueue(&queue);

    // Print results
    printf("%ld %ld %ld %ld\n", sum, odd, min, max);

    // Clean up and return
    free(threads);
    return (EXIT_SUCCESS);
}
