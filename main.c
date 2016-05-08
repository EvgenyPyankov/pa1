#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>


#include "common.h"
#include "ipc.h"
#include "pa1.h"

// http://cboard.cprogramming.com/c-programming/119168-sending-structs-pointers-through-pipes.html

enum { NUM_CHILDREN = 10 }; ///
enum { NUM_MESSAGES = 10 }; ///

enum { MAX_NUMBER_OF_CHILDREN = 10 };

static int write_pipes[MAX_NUMBER_OF_CHILDREN + 1][MAX_NUMBER_OF_CHILDREN + 1];
static int read_pipes[MAX_NUMBER_OF_CHILDREN + 1][MAX_NUMBER_OF_CHILDREN + 1];

static const char * const  new_pipe_fmt = "New pipe has been created(%d,%d)\n";

int childrenNumber;

int eventsLogDescriptor;
int pipesLogDescriptor;

void pipeWrite(int src, int dest, Message* msg){
    write(write_pipes[src][dest], &msg, sizeof(msg));
}

int receive(void * self, local_id from, Message * msg){
	int dest = (int)self;
	int nbytes = read(read_pipes[dest][from], msg, sizeof(*msg));
	if (nbytes >= 0) {  /// ???
		return 0;
	}

	return -1;
}

int receive_any(void * self, Message * msg){
	int dest = (int)self;
	while (1) {
		for (int i = 1; i <= childrenNumber; i ++) {
			if (dest != i) {
				if (receive((void *)dest, i, msg) == 0) {
					return 0;
				}
			}
		}
	}

	return -1;
}

int send(void * self, local_id dst, const Message * msg){
	int src = (int)self;
    int nbytes = write(write_pipes[src][dst], msg, sizeof(*msg));
    if (nbytes != -1) {
    	return 0;
    }

    return -1;
}

int send_multicast(void * self, const Message * msg){
	int src = (int)self;
	for (int i = 0; i <= childrenNumber; i ++) {
		if (i != src) {
			int result = send((void *)src, i, msg);
			if (result != 0) {
				return -1;
			}
		}
	}

	return 0;
}

void logWrite(int descriptor, char buf[])
{
    write(descriptor, buf, strlen(buf));
    printf("%.*s",strlen(buf),buf);
}

void be_childish(int id) ////
{
	close(read_pipes[id][0]);
	int pid = getpid();
	int parentProcessId = getppid();
	
	char buf[64];
	sprintf(buf, log_started_fmt, id, pid, parentProcessId);
    logWrite(eventsLogDescriptor, buf);

    MessageHeader header = {MESSAGE_MAGIC, strlen(buf), STARTED, (int)time(NULL)};
    Message message;
    message.s_header = header;
    sprintf(message.s_payload, buf, strlen(buf));
    if (send_multicast((void *)id, &message) != 0) {
    	printf("send_multicast() failed");
    }

    int numberOfReceivedStartedMessages = 0;
    int numberOfReceivedDoneMessages = 0;
    while (numberOfReceivedStartedMessages < childrenNumber - 1) {
    	Message* msg = malloc(sizeof(*msg));
    	if (receive_any((void *)id, msg) != 0) {
    		printf("receive_any() failed");
    	}
    	printf("%d received message: %s", id, msg[0].s_payload);
    	if (msg[0].s_header.s_type == STARTED) {
    		numberOfReceivedStartedMessages = numberOfReceivedStartedMessages + 1;
    	} else if (msg[0].s_header.s_type == DONE) {
    		numberOfReceivedDoneMessages = numberOfReceivedDoneMessages + 1;
    	}
    }
    sprintf(buf, log_received_all_started_fmt, id);
    logWrite(eventsLogDescriptor, buf);
    exit(0);
}

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

void openLogFiles(){
	eventsLogDescriptor = open(events_log, O_WRONLY | O_APPEND | O_CREAT, 0666);
	pipesLogDescriptor = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT, 0666);
}

void closeLogFiles(){
	close(eventsLogDescriptor);
	close(pipesLogDescriptor);
}



void prepare(){
    remove(events_log);
    remove(pipes_log);
    openLogFiles();
}

void shutdown(){
    closeLogFiles();
}



int main(int argc, char **argv) {
	childrenNumber = atoi(argv[2]);


	pid_t pid;
    int i, j;

    prepare();

    /* Create pipes */
    for (i = 0; i < childrenNumber + 1; i ++)
    {
    	for (j = 0; j < childrenNumber + 1; j ++)
    	{
    		if (i != j) {
	    		int new_pipe[2];
	    		if (pipe(new_pipe))
		        {
		            int errnum = errno;
		            fprintf(stderr, "Pipe failed (%d: %s)\n", errnum, strerror(errnum));
		            return EXIT_FAILURE;
		        }
	    		read_pipes[j][i] = new_pipe[0];
	    		fcntl(read_pipes[j][i], F_SETFL, O_NONBLOCK);
	    		write_pipes[i][j] = new_pipe[1];
	            // char buf[64];
	            // sprintf(buf, new_pipe_fmt, new_pipe[0],new_pipe[1]);
	            // logWrite(pipesLogDescriptor, buf);
	            // sprintf(buf, new_pipe_fmt, new_pipe[1],new_pipe[0]);
	            // logWrite(pipesLogDescriptor, buf);
	        }
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
    		close(write_pipes[0][i]);
        }
    }

// Message* msg = malloc(sizeof(*msg));
		    // // for (int i=1; i <= childrenNumber; i++){
		    // // 	printf("%d\n", read_pipes[0][i]);
		    // 	char buffer[64];
	// if (receive((void *)0, 3, &message) != 0 ) {
	// 	printf("receive() failed");
	// }
	
	// read(read_pipes[0][2], msg, sizeof(*msg));
// receive_any((void *)0, msg);
// printf("Message: %s", msg[0].s_payload);

	// Message* msg2 = malloc(sizeof(*msg2));
	// if (receive((void *)0, 2, msg2) != 0){
	// 	printf("second message is not received()");
	// }
 //        printf("%s\n", buffer);
 //    // }

    while(wait(NULL)>0) {
    	Message* msg = malloc(sizeof(*msg));
    	receive_any((void *)0, msg);
    	printf("0 received message: %s", msg[0].s_payload);
    }


    printf("Parent complete\n");

    shutdown();

    return 0;
 
}
