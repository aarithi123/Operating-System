#include <errno.h>
#include <sys/klog.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include "process_log/process_log.h"
}

using std::cout;
using std::endl;

void wait_for_dmesg(int timeout_s) {
    auto now = std::chrono::system_clock::now();
    size_t len = klogctl(10, NULL, 0);
    while (std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now() - now)
               .count() < timeout_s) {
        if (klogctl(10, NULL, 0) != len)
            return;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// get and clear dmesg
std::string get_dmesg() {
    size_t len;  // read dmesg ring buffer size
    for (int i = 0; i < 1000; i++) {
        len = klogctl(10, NULL, 0);
    }

    if (len == -1) {
        std::cout << "Error reading dmesg " << errno << std::endl;
        return "";
    } else if (len == 0) {
        return "";
    }

    std::vector<char> buf(len, 0);

    len = klogctl(4, &buf[0], len);  // read and clear dmesg ring buffer
    if (len == -1) {
        std::cout << "Error reading dmesg " << errno << std::endl;
        return "";
    }

    buf.resize(len);
    return std::string(buf.begin(), buf.end());
}

bool message_in_dmesg(std::string message) {
    std::string dmesg = get_dmesg();
    std::stringstream dmesg_stream(dmesg);

    std::string line;
    while (std::getline(dmesg_stream, line)) {
        if (line.find(message) != std::string::npos) {
            // Print out the detected message
            std::cout << line << std::endl;
            return true;
        }
    }

    return false;
}

int harness_get_proc_log_level() {
    int* get_level_params = retrieve_get_level_params();

    int ret = -1;

    switch (get_level_params[1]) {
        case 0:
            ret = syscall(get_level_params[0]);
            break;
        case 1:
            ret = syscall(get_level_params[0], get_level_params[2]);
            break;
        case 2:
            ret = syscall(get_level_params[0], get_level_params[2],
                          get_level_params[3]);
            break;

        default:
            break;
    }

    free(get_level_params);

    return interpret_get_level_result(ret);
}

int harness_set_proc_log_level(int new_level) {
    int* set_level_params = retrieve_set_level_params(new_level);

    int ret = -1;

    switch (set_level_params[1]) {
        case 0:
            ret = syscall(set_level_params[0]);
            break;
        case 1:
            ret = syscall(set_level_params[0], set_level_params[2]);
            break;
        case 2:
            ret = syscall(set_level_params[0], set_level_params[2],
                          set_level_params[3]);
            break;

        default:
            break;
    }

    free(set_level_params);

    return interpret_set_level_result(ret);
}

int harness_proc_log_message(int level, char* message) {
    int ret = syscall(PROC_LOG_CALL, message, level);
    return interpret_log_message_result(ret);
}

// if running as superuser this command will change process permission
// level to that of a normal user process
bool drop_privileges() {
    if (setresgid(1000, 1000, 0) == -1 || setresuid(1000, 1000, 0) == -1) {
        /* Failed: no root privileges! */
        return false;
    }
    return true;
}

// if process originally started as superuser, this command will change
// process permission level back to that of the superuser
bool re_elevate_privileges() {
    if (setresgid(0, 0, 0) == -1 || setresuid(0, 0, 0) == -1) {
        /* Failed: no root privileges! */
        return false;
    }
    return true;
}

int tester_main(int argc, char** argv) {
    // Clear dmesg ring buffer
    re_elevate_privileges();
    get_dmesg();
    drop_privileges();

    cout
        << "COP4600 P1 Harness Test\nNote: These tests ARE NOT COMPREHENSIVE, "
           "you should write your own tests as well to check functionality.\n\n"
        << endl;

    cout << "Set system log level to 3 as normal user (should fail)" << endl;
    cout << "Log level before change: " << harness_get_proc_log_level() << endl;
    cout << "Harness set_proc_log_level return output: "
         << harness_set_proc_log_level(3) << endl;
    cout << "Log level after change: " << harness_get_proc_log_level() << endl;
    cout << endl;

    re_elevate_privileges();
    cout << "Set system log level to 3 as superuser (should succeed)" << endl;
    cout << "Log level before change: " << harness_get_proc_log_level() << endl;
    cout << "Harness set_proc_log_level return output: "
         << harness_set_proc_log_level(3) << endl;
    cout << "Log level after change: " << harness_get_proc_log_level() << endl;
    drop_privileges();
    cout << endl;

    std::string message = "Test print message. Linux Rocks!";
    cout << "Print message with level 2 while in system log level 3 (should "
            "succeed)"
         << endl;
    cout << "Harness proc_log_message return output: "
         << harness_proc_log_message(2, const_cast<char*>(message.c_str()))
         << endl;
    wait_for_dmesg(5);
    re_elevate_privileges();
    bool printed = message_in_dmesg(message);
    drop_privileges();
    cout << "Did the message print?: " << ((printed) ? "Yes" : "No") << endl;
    cout << endl;

    re_elevate_privileges();
    harness_set_proc_log_level(2);
    drop_privileges();
    message = "Test print message. Linux Rocks!";
    cout << "Print message with level 3 while in system log level 2 (should "
            "not print)"
         << endl;
    cout << "Harness proc_log_message return output: "
         << harness_proc_log_message(3, const_cast<char*>(message.c_str()))
         << endl;
    wait_for_dmesg(5);
    re_elevate_privileges();
    printed = message_in_dmesg(message);
    drop_privileges();
    cout << "Did the message print?: " << ((printed) ? "Yes" : "No") << endl;
    cout << endl;

    re_elevate_privileges();
    harness_set_proc_log_level(4);
    drop_privileges();
    message = "Test print message. Linux Rocks!";
    cout << "Print message with level -1 while in system log level 4 "
            "(should "
            "fail)"
         << endl;
    cout << "Harness proc_log_message return output: "
         << harness_proc_log_message(-1, const_cast<char*>(message.c_str()))
         << endl;
    wait_for_dmesg(5);
    re_elevate_privileges();
    printed = message_in_dmesg(message);
    drop_privileges();
    cout << "Did the message print?: " << ((printed) ? "Yes" : "No") << endl;
    cout << endl;

    re_elevate_privileges();
    harness_set_proc_log_level(4);
    drop_privileges();
    char* badAddress = (char*)0xffffffffffffffff;
    cout << "Testing safe message copying with copy_from_user (may crash your VM)..."
         << endl;
    cout << "Harness proc_log_message return output: "
         << harness_proc_log_message(2, badAddress) << endl;
    wait_for_dmesg(3);
    cout << "Congratulations! You did not crash." << endl;
    cout << endl;

    return 0;
}

int main(int argc, char** argv) {
    if (getuid() != 0 || geteuid() != 0) {
        cout << "Attempting to elevate to superuser ...\n\n" << endl;
        // get path to executable
        constexpr int PATH_BUF_SIZE = 1024;
        std::vector<char> buf(PATH_BUF_SIZE, 0);
        ssize_t path_str_len =
            readlink("/proc/self/exe", &buf[0], PATH_BUF_SIZE);
        buf[path_str_len] = '\0';
        size_t argv_len = 0;
        for (unsigned int i = 0; i < argc; i++) {
            argv_len += std::strlen(argv[i]);
        }
        std::stringstream args_stream;
        for (unsigned int i = 1; i < argc; i++) {
            args_stream << " " << argv[i];
        }
        std::stringstream path_stream;
        path_stream << "sudo "
                    << "\"" << std::string(&buf[0]) << "\""
                    << args_stream.str();
        // re-execute
        int ret = system(path_stream.str().c_str());
        return ret;
    } else {
        return tester_main(argc, argv);
    }
}