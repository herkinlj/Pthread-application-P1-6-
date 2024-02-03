/*
 * par_sum.c
 *
 * CS 470 Project 1 (Pthreads)
 * Serial version
 *
 * Compile with --std=c99
 * Authors : Liam J Herkins & Jared Householder
 */

/**
 * Did you use an AI-assist tool while constructing your solution? In what ways was it helpful (or not)?
 *
Yes, when using AI it helped create the base of the code that I needed in order to get started with the project. However, there were issues with the code that I noticed. It tried sticking the condition variables and the mutex inside of the TaskQueue struct (which is the singly linked list to queue the tasks based on the input) which causes trouble because mutexes and condition variables are typically shared among threads to provide synchronization. If each TaskQueue instance has its own set of mutex and condition variables, threads trying to lock the mutex or signal on condition variables won't be synchronized across different instances. Therefore, they need to be global variables. Also, in the worker_function, it tried to process the tasks (whether it needed to update or sleep) inside of the critical section that is protected by the mutexes, which obviously is not needed as long as the update function is protected appropriately. Reason being is that we want to minimize portions of the code to be protected by a mutex to allow for more parallelism.

How did you verify that your solution's performance scales linearly with the number of threads? Describe your experiments in detail.

I shall break this analysis of linearity into two parts: with and without the calls to sleep() in the program.  In the first series of tests, we ran the tests on a text file with 400,000 lines (four part sequence from writeup repeated 100k times).  We ran each thread count 10 times to account for background noise and the heavy workload done on the cluster as it was close to submission time.  Without the calls to sleep, we can confidently say the performance does strongly scale (inverse) but does not appear to possess any linearity.  I use the term inverse as as the number of threads increases, the overhead increases, causing an increase in runtime with each incrementation (powers of two) of the thread count.
For the implementation with calls to sleep(), I chose a sample size of 64 tests ( sequences).  The solution we have formulated does appear to scale linearly until it reaches a lower bound.  This bound is reached at 32 threads for which it bottoms out.  Though it does scale strongly, without a larger sample size, I cannot prove its absolute linearity.  However, I can argue that linearity is present up to 16 threads.  The increase in core count (count 2 was omitted) is inversely proportional to the runtime with a margin of error due to noise that I would consider satisfactory.  For reference linearity is n/m = 1, and with the samples collected, it would be +- .09 of that measure.  The provided graphs support these findings.
https://docs.google.com/spreadsheets/d/1tG-qqB8snnqbzwIHisyqXfMB7o4ndq7gumzFLvst6uA/edit?usp=sharing

How does your solution ensure the worker threads did not terminate until all tasks had entered the system?
Our solution ensures that worker threads do not terminate until all tasks have entered the system by using a condition variable (not_empty). Worker threads wait on this variable, and they only exit when the queue is both empty and marked as done.
How does your solution ensure that all of the worker threads are terminated cleanly when the supervisor is done?
The done flag in the task queue signals that no more tasks will be added. After setting “done” to true, the code broadcasts to all worker threads using pthread_cond_broadcast(&not_empty). This wakes up any waiting threads, allowing them to check the done flag and exit cleanly.


Suppose that we wanted a priority-aware task queue. How would this affect your queue implementation, and how would it affect the threading synchronization?

The priority-aware task queue would likely introduce additional complexity to synchronization. The critical sections for enqueue and dequeue operations would need to be carefully managed to avoid race conditions, especially when modifying the queue structure based on priority. Basically it would introduce more intricate synchronization to maintain correctness and efficiency. When enqueuing a task, we would need to consider its priority and insert it at the appropriate position in the queue, ensuring that higher-priority tasks are placed ahead of lower-priority ones. The dequeue operation would need to select tasks based on their priority level. This might involve altering the current logic to prioritize higher-priority tasks over lower-priority ones.

Suppose that we wanted task differentiation (e.g., some tasks can only be handled by some workers). How would this affect your solution?

Synchronization mechanisms would need to be enhanced to handle the nuanced task execution based on the differentiation. This might involve additional data structures or synchronization points to manage task types and worker eligibility. For example, Each task might need an additional attribute indicating the type of worker it requires or the category it falls into. Worker threads would need to check if they are eligible to process a particular task based on its type before executing it. Also, when enqueuing a task, the type or required worker information needs to be considered. The enqueue operation must ensure that the task is placed in a manner consistent with the task-worker affinity. Furthermore, the dequeue operation must select tasks based on their types and worker affinity. Workers should only process tasks that match their capabilities.

 *
 *
 *
 *
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
