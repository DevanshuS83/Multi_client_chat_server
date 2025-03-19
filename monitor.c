#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "monitor.h"

// Global variable (if needed) could be defined here,
// but since the server already has its own 'client_count',
// we'll simply print the updated count.

void update_client_count(int count) {
    // This function can be expanded to do more (e.g., log or update shared status)
    printf("Updated client count: %d\n", count);
}

void *monitor_connections(void *arg) {
    while (1) {
        // In a more complex version, we might display stats here.
        // For now, we'll simply sleep.
        sleep(5);
    }
    return NULL;
}

void start_monitoring() {
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_connections, NULL);
    pthread_detach(monitor_thread);
}
