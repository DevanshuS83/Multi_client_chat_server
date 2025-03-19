#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "logger.h"

static char log_filename[256] = "chat.log";  // default log file name
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

void init_logger(const char *filename) {
    // Copy the filename into our static variable
    strncpy(log_filename, filename, sizeof(log_filename) - 1);
    log_filename[sizeof(log_filename) - 1] = '\0';
}

void log_message(const char *format, ...) {
    FILE *fp;
    time_t now;
    char time_str[20];
    va_list args;

    pthread_mutex_lock(&log_lock);

    fp = fopen(log_filename, "a");
    if (fp == NULL) {
        perror("Error opening log file");
        pthread_mutex_unlock(&log_lock);
        return;
    }

    now = time(NULL);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(fp, "[%s] ", time_str);

    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);

    fprintf(fp, "\n");
    fclose(fp);

    pthread_mutex_unlock(&log_lock);
}

void close_logger() {
    // In this implementation, we open and close the file in each call,
    // so there's nothing persistent to close.
}
