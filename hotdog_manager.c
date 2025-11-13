#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Structure to represent a hot dog
typedef struct {
    int hot_dog_id;
    char making_machine_id[32];  // Buffer to store machine ID (e.g., "m1", "m30", etc.)
} HotDog;

// Global variables for command-line arguments
int N;  // Number of hot dogs to make
int S;  // Number of slots in the pool
int M;  // Number of making machines
int P;  // Number of packing machines

// Circular buffer (pool) for hot dogs
HotDog *pool;
int pool_front = 0;      // Front index of the queue
int pool_rear = 0;       // Rear index of the queue
int pool_count = 0;      // Current number of hot dogs in the pool

// Synchronization primitives for pool access
pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pool_not_full = PTHREAD_COND_INITIALIZER;   // Signal when pool has space
pthread_cond_t pool_not_empty = PTHREAD_COND_INITIALIZER;  // Signal when pool has items

// Synchronization for file logging
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global counters for tracking production
int hot_dogs_made = 0;      // Total number of hot dogs made
int hot_dogs_packed = 0;    // Total number of hot dogs packed
int next_hot_dog_id = 1;    // Next hot dog ID to assign
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

// Statistics arrays for summary
int *made_by_machine;       // Array to track hot dogs made by each making machine
int *packed_by_machine;     // Array to track hot dogs packed by each packing machine

// Function to simulate work (n units of time)
void do_work(int n) {
    for (int i = 0; i < n; i++) {
        long m = 300000000L;
        while (m > 0) m--;
    }
}

// Function to log to file (thread-safe)
void log_to_file(const char *message) {
    pthread_mutex_lock(&log_mutex);
    FILE *file = fopen("log.txt", "a");
    if (file) {
        fprintf(file, "%s\n", message);
        fclose(file);
    }
    pthread_mutex_unlock(&log_mutex);
}

// Making machine thread function (Producer)
void *making_machine(void *arg) {
    int machine_num = *(int *)arg;
    char machine_id[32];
    sprintf(machine_id, "m%d", machine_num);
    
    while (1) {
          do_work(4);
          // Wait if pool is full
          pthread_mutex_lock(&pool_mutex);
        while (pool_count >= S) {
            pthread_cond_wait(&pool_not_full, &pool_mutex);
        }
        pthread_mutex_unlock(&pool_mutex);
                // Send to pool (1 unit of time)
            do_work(1);
        // Check if we should stop (all N hot dogs have been made)
        pthread_mutex_lock(&counter_mutex);
        if (hot_dogs_made >= N) {
            pthread_mutex_unlock(&counter_mutex);
            break;
        }
        
        // Get the next hot dog ID and increment counters
        int current_hot_dog_id = next_hot_dog_id++;

        hot_dogs_made++;
        made_by_machine[machine_num - 1]++;

        pthread_mutex_unlock(&counter_mutex);
        

        
        // Create hot dog structure
        HotDog hotdog;
        hotdog.hot_dog_id = current_hot_dog_id;
        strcpy(hotdog.making_machine_id, machine_id);
        

        
        // Add hot dog to pool (with synchronization)
        pthread_mutex_lock(&pool_mutex);
        
        
        
        // Add hot dog to the end of the queue (circular buffer)
        pool[pool_rear] = hotdog;
        pool_rear = (pool_rear + 1) % S;
        pool_count++;
        
        // Log the making action (recheck logic)
        char log_message[100];
        sprintf(log_message, "%s puts %d", machine_id, current_hot_dog_id);
        log_to_file(log_message);
        
        // Signal that pool is not empty (wake up packing machines)
        pthread_cond_signal(&pool_not_empty);
        pthread_mutex_unlock(&pool_mutex);
    }
    
    return NULL;
}

// Packing machine thread function (Consumer)
void *packing_machine(void *arg) {
    int machine_num = *(int *)arg;
    char machine_id[32];
    sprintf(machine_id, "p%d", machine_num);
    
    while (1) {
        // Check if we should stop (all N hot dogs have been packed)
        pthread_mutex_lock(&counter_mutex);
        if (hot_dogs_packed >= N) {
            pthread_mutex_unlock(&counter_mutex);
            break;
        }
        pthread_mutex_unlock(&counter_mutex);
        
        // Take hot dog from pool (with synchronization)
        HotDog hotdog;
        pthread_mutex_lock(&pool_mutex);
        
        // Wait if pool is empty (MUST hold mutex before checking and waiting)
        while (pool_count == 0) {
            // Check again if we should stop after waking up
            pthread_mutex_lock(&counter_mutex);
            if (hot_dogs_packed >= N) {
                pthread_mutex_unlock(&counter_mutex);
                pthread_mutex_unlock(&pool_mutex);
                return NULL;
            }
            pthread_mutex_unlock(&counter_mutex);
            
            pthread_cond_wait(&pool_not_empty, &pool_mutex);
        }
        
        // Take hot dog from the front of the queue (circular buffer)
        hotdog = pool[pool_front];
        pool_front = (pool_front + 1) % S;
        pool_count--;
        
        // Signal that pool is not full (wake up making machines)
        pthread_cond_signal(&pool_not_full);
        pthread_mutex_unlock(&pool_mutex);
        
        // Take from pool (1 unit of time) - happens AFTER taking from pool
        do_work(1);
        
        // Pack hot dog (2 units of time)
        do_work(2);
        
        // Update counters and statistics
        pthread_mutex_lock(&counter_mutex);
        hot_dogs_packed++;
        packed_by_machine[machine_num - 1]++;
        pthread_mutex_unlock(&counter_mutex);
        
        // Log the packing action
        char log_message[100];
        sprintf(log_message, "%s gets %d from %s", machine_id, 
                hotdog.hot_dog_id, hotdog.making_machine_id);
        log_to_file(log_message);
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments
    if (argc != 5) {
        fprintf(stderr, "Usage: %s N S M P\n", argv[0]);
        return 1;
    }
    
    N = atoi(argv[1]);
    S = atoi(argv[2]);
    M = atoi(argv[3]);
    P = atoi(argv[4]);
    
    // Validate inputs
    if (N <= 0 || S <= 0 || M <= 0 || P <= 0 || 
        M > 30 || P > 30 || S >= N) {
        fprintf(stderr, "Invalid arguments: 1 <= M, P <= 30 and S < N\n");
        return 1;
    }
    
    // Initialize log file
    FILE *file = fopen("log.txt", "w");
    if (!file) {
        perror("Error creating log file");
        return 1;
    }
    fprintf(file, "order:%d\n", N);
    fprintf(file, "capacity:%d\n", S);
    fprintf(file, "making machines:%d\n", M);
    fprintf(file, "packing machines:%d\n", P);
    fprintf(file, "-----\n");
    fclose(file);
    
    // Allocate memory for pool
    pool = (HotDog *)malloc(S * sizeof(HotDog));
    if (!pool) {
        perror("Memory allocation failed");
        return 1;
    }
    
    // Allocate memory for statistics
    made_by_machine = (int *)calloc(M, sizeof(int));
    packed_by_machine = (int *)calloc(P, sizeof(int));
    if (!made_by_machine || !packed_by_machine) {
        perror("Memory allocation failed");
        return 1;
    }
    
    // Create making machine threads
    pthread_t *making_threads = (pthread_t *)malloc(M * sizeof(pthread_t));
    int *making_nums = (int *)malloc(M * sizeof(int));
    if (!making_threads || !making_nums) {
        perror("Memory allocation failed");
        return 1;
    }
    
    for (int i = 0; i < M; i++) {
        making_nums[i] = i + 1;
        if (pthread_create(&making_threads[i], NULL, making_machine, 
                          &making_nums[i]) != 0) {
            perror("Failed to create making thread");
            return 1;
        }
    }
    
    // Create packing machine threads
    pthread_t *packing_threads = (pthread_t *)malloc(P * sizeof(pthread_t));
    int *packing_nums = (int *)malloc(P * sizeof(int));
    if (!packing_threads || !packing_nums) {
        perror("Memory allocation failed");
        return 1;
    }
    
    for (int i = 0; i < P; i++) {
        packing_nums[i] = i + 1;
        if (pthread_create(&packing_threads[i], NULL, packing_machine, 
                          &packing_nums[i]) != 0) {
            perror("Failed to create packing thread");
            return 1;
        }
    }
    
    // Wait for all making threads to complete
    for (int i = 0; i < M; i++) {
        pthread_join(making_threads[i], NULL);
    }
    
    // Wait for all packing threads to complete
    for (int i = 0; i < P; i++) {
        pthread_join(packing_threads[i], NULL);
    }
    
    // Append summary to log file
    file = fopen("log.txt", "a");
    if (file) {
        fprintf(file, "-----\n");
        fprintf(file, "summary:\n");
        for (int i = 0; i < M; i++) {
            fprintf(file, "m%d made %d\n", i + 1, made_by_machine[i]);
        }
        for (int i = 0; i < P; i++) {
            fprintf(file, "p%d packed %d\n", i + 1, packed_by_machine[i]);
        }
        fclose(file);
    }
    
    // Cleanup
    free(pool);
    free(made_by_machine);
    free(packed_by_machine);
    free(making_threads);
    free(making_nums);
    free(packing_threads);
    free(packing_nums);
    
    pthread_mutex_destroy(&pool_mutex);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&counter_mutex);
    pthread_cond_destroy(&pool_not_full);
    pthread_cond_destroy(&pool_not_empty);
    
    return 0;
}

