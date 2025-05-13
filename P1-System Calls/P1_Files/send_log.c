/* setlevel.c
	by Jeremiah Blanchard
	2017/09/25
	allows shell command to set security level for project 1 in COP4600
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_log/process_log.h"

#define ARGCNT 3
#define HELPMSG "Usage: send_log <level> <message>\n"

int main(int argc, char** argv)
{
    int level;
    int retval;

    // check and get command line arguments
    if (argc < ARGCNT)
    {
        printf("%s", "Not enough parameters\n");
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

    // attempt call
    level = proc_log_message(level, argv[2]);
    if (level == -1)
    {
        printf("%s", "proc_log_message failed\n");
        printf("%s", HELPMSG);
        return 0;
    }

    // print result if success
    printf("Successfully logged message [%s]; returned %d\n", argv[2], level);
    return 0;
}
