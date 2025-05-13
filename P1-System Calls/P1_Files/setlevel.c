/* setlevel.c
	by Jeremiah Blanchard
	2017/09/25
	allows shell command to set security level for project 1 in COP4600
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_log/process_log.h"

#define ARGCNT 2
#define HELPMSG "Usage: setlevel <level>\n"

int main(int argc, char** argv)
{
    int level;
    int retval;

    // check and get command line arguments
    if (argc < ARGCNT)
    {
        printf("%s", "No level parameter\n");
        printf("%s", HELPMSG);
        return 0;
    }

    retval = sscanf(argv[1], "%d", &level);
    if (retval != 1)
    {
        printf("%s", "Cannot parse level\n");
        printf("%s", HELPMSG);
        return 0;
    }

    // advise on non-root process
    retval = geteuid();
    if (retval != 0)
    {
        printf("%s", "NOTE: not root user\n");
    }

    // attempt call
    level = set_proc_log_level(level);
    if (level == -1)
    {
        printf("%s", "set_proc_log_level failed\n");
        printf("%s", HELPMSG);
        return 0;
    }

    // print result if success
    printf("Successfully set process log level: %d\n", level);
    return 0;
}
