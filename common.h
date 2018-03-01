#ifndef COMMON_H
#define COMMON_H
#include "errno.h"
#include <stdio.h>

#define DEBUG 1
#define BUFFSIZE 4096
// macro to simplify error handling
#define GENERIC_ERROR_HELPER(cond, errCode, msg) do {               \
        if (cond) {                                                 \
            fprintf(stderr, "%s: %s\n", msg, strerror(errCode));    \
            exit(EXIT_FAILURE);                                     \
        }                                                           \
    } while(0)

#define ERROR_HELPER(ret, msg)          GENERIC_ERROR_HELPER((ret < 0), errno, msg)
#define PTHREAD_ERROR_HELPER(ret, msg)  GENERIC_ERROR_HELPER((ret != 0), ret, msg)

/**
#define debug_printf(fmt, ...) \
           do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)
**/

#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

/* Configuration parameters */
#define SERVER_ADDRESS  "127.0.0.1"
#define SERVER_COMMAND  "QUIT"
#define SERVER_PORT     8888

#endif

