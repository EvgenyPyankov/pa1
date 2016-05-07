#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>


#include "common.h"
#include "ipc.h"
#include "pa1.h"

enum { NUM_CHILDREN = 10 }; ///
enum { NUM_MESSAGES = 10 }; ///

enum { MAX_NUMBER_OF_CHILDREN = 10 };

static int write_pipes[MAX_NUMBER_OF_CHILDREN + 1][MAX_NUMBER_OF_CHILDREN + 1];
static int read_pipes[MAX_NUMBER_OF_CHILDREN + 1][MAX_NUMBER_OF_CHILDREN + 1];

static int n_pipes; ///


int childrenNumber;

FILE* eventsLog;
FILE* pipesLog;

static void be_childish(int id) ////
{
	close(read_pipes[id][0]);
	int pid = getpid();
	int eventsLogDescriptor = open(events_log, O_WRONLY | O_APPEND);
	char buf[64];
	sprintf(buf, log_started_fmt, id, pid, id);
	write(eventsLogDescriptor, buf, strlen(buf));
	// "Child %d started\n", id);
    // int i;
    // char buffer[32];
    // int nbytes;
    // int pid = getpid();
    // close(pipe[1]);
    // for (i = 0; i < n_pipes; i++)
    //     close(write_pipes[i]);
    // printf("Child %d\n", pid);
    // while ((nbytes = read(pipe[0], buffer, sizeof(buffer))) > 0)
    // {
    //     printf("Child %d: %d %.*s\n", pid, nbytes, nbytes, buffer);
    //     fflush(0);
    // }
    // printf("Child %d: finished\n", pid);
    exit(0);
}

void openLogFiles(){
	eventsLog = fopen(events_log,"w+");
	pipesLog = fopen(pipes_log,"w+");
}

void closeLogFiles(){
	fclose(eventsLog);
	fclose(pipesLog);
}

void pipesLogWrite(char* line)
{
	fprintf(pipesLog, "%s", line);
	fflush(pipesLog);
}

void eventsLogWrite(char* line)
{
	fprintf(eventsLog, "%s", line);
	fflush(eventsLog);
}

int main(int argc, char **argv) {
	childrenNumber = atoi(argv[2]);

	pid_t pid;
    int i, j;

    /* Create pipes */
    for (i = 0; i < childrenNumber + 1; i ++)
    {
    	for (j = 0; j < i; j ++)
    	{
    		int new_pipe[2];
    		if (pipe(new_pipe))
	        {
	            int errnum = errno;
	            fprintf(stderr, "Pipe failed (%d: %s)\n", errnum, strerror(errnum));
	            return EXIT_FAILURE;
	        }
    		read_pipes[i][j] = new_pipe[0];
    		read_pipes[j][i] = new_pipe[0];
    		write_pipes[i][j] = new_pipe[1];
    		write_pipes[j][i] = new_pipe[1];
    	}
    }

    /* Create children. */
    for (i = 1; i <= childrenNumber; i ++)
    {
    	pid = fork();

    	if (pid < 0)
        {
            int errnum = errno;
            fprintf(stderr, "Fork failed (%d: %s)\n", errnum, strerror(errnum));
            return EXIT_FAILURE;
        }

        if (pid == 0)
        {
        	be_childish(i);
        } else {
        	for (j = 1; j < childrenNumber; j ++)
        	{
        		close(write_pipes[0][j]);
        	}
        }
    }


    while(wait(NULL)>0) {
    }

    // for (i = 0; i < NUM_CHILDREN; i++)
    // {
    //     int new_pipe[2];
    //     if (pipe(new_pipe))
    //     {
    //         int errnum = errno;
    //         fprintf(stderr, "Pipe failed (%d: %s)\n", errnum, strerror(errnum));
    //         return EXIT_FAILURE;
    //     }
    //     if ((pid = fork()) < 0)
    //     {
    //         int errnum = errno;
    //         fprintf(stderr, "Fork failed (%d: %s)\n", errnum, strerror(errnum));
    //         return EXIT_FAILURE;
    //     }
    //     else if (pid == 0)
    //     {
    //         be_childish(new_pipe);
    //     }
    //     else
    //     {
    //         close(new_pipe[0]);
    //         write_pipes[n_pipes++] = new_pipe[1];
    //     }
    // }

    // for (i = 0; i < NUM_MESSAGES; i++)
    // {
    //     char message[30];
    //     int len;
    //     snprintf(message, sizeof(message), "Message %d", i);
    //     len = strlen(message);
    //     for (j = 0; j < n_pipes; j++)
    //     {
    //         if (write(write_pipes[j], message, len) != len)
    //         {
    //             /* Inferior error handling; first failure causes termination */
    //             fprintf(stderr, "Write failed (child %d)\n", j);
    //             exit(1);
    //         }
    //     }
    //     sleep(1);
    // }
    printf("Parent complete\n");

    return 0;
    // 
    // 
    // 
	// childrenNumber = atoi(argv[2]);
 //    printf("p = %d\n", childrenNumber);
	// const char *buf = "123456789";
	// openLogFiles();
	// fprintf(eventsLog, "Some text:%s", buf);
	// closeLogFiles();
	// return 0;
}
