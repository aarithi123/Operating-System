#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "process_log.h"

// get the process log level
int get_proc_log_level() {
    // system call to get log level
    return syscall(GET_LOG_LEVEL);
}

// set the process log level
int set_proc_log_level(int new_level) {
    // system call to set log level
    return syscall(SET_LOG_LEVEL, new_level);
}

// log a message
int proc_log_message(int level, char *message) {
    // system call to log a message
    return syscall(PROC_LOG_CALL, message, level);
}

// Harness functions
// retrieve parameters to set log level
int* retrieve_set_level_params(int new_level) {
    int* params = (int*)malloc(3 * sizeof(int));
    if (params != NULL) {
        params[0] = SET_LOG_LEVEL; // system call number
        params[1] = 1;             // number of parameters
        params[2] = new_level;     // new log level
    }
    return params;
}

// retrieve parameters to get log level
int* retrieve_get_level_params() {
    int* params = (int*)malloc(2 * sizeof(int));
    if (params != NULL) {
        params[0] = GET_LOG_LEVEL; // system call number
        params[1] = 0;             // number of parameters
    }
    return params;
}

// system call's return value to set log level
int interpret_set_level_result(int ret_value) {
    return ret_value;
}

// system call's return value to get log level
int interpret_get_level_result(int ret_value) {
    return ret_value;
}

// system call's return value to log a message
int interpret_log_message_result(int ret_value) {
    return ret_value;
}

