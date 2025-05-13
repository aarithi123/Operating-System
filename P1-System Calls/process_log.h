#ifndef PROCESS_LOG_H
#define PROCESS_LOG_H

#define GET_LOG_LEVEL 435 // syscall number get_proc_log_level
#define SET_LOG_LEVEL 436 // syscall number set_proc_log_level
#define PROC_LOG_CALL 437 // syscall number proc_log_message

int get_proc_log_level();
int set_proc_log_level(int new_level);
int proc_log_message(int level, char *message);

// Harness functions
int* retrieve_set_level_params(int new_level);
int* retrieve_get_level_params();
int interpret_set_level_result(int ret_value);
int interpret_get_level_result(int ret_value);
int interpret_log_message_result(int ret_value);

#endif // PROCESS_LOG_H
