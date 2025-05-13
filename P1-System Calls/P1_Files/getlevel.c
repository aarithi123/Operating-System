/* getlevel.c
	by Jeremiah Blanchard
	2020/09/03
	allows shell command to read process log level for project 1 in COP4600
 */

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_log/process_log.h"

#define ARGCNT 1
#define HELPMSG "Usage: getlevel\n"

int main(int argc, char** argv)
{
    int level;

    // attempt call
    level = get_proc_log_level();
    if (level == -1)
    {
        printf("%s", "get_proc_log_level failed\n");
        printf("%s", HELPMSG);
        return 0;
    }

    // print result if success
    printf("Successfully retrieved process log level: %d\n", level);
    return 0;
}
