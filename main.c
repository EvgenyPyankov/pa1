#define _BSD_SOURCE
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
#include <stdint.h>
#include <unistd.h>


#include "common.h"
#include "ipc.h"
#include "pa1.h"

#define INT2VOIDP(i) (void*)(uintptr_t)(i)


enum { MAX_NUMBER_OF_CHILDREN = 10 };

static int write_pipes[MAX_NUMBER_OF_CHILDREN + 1][MAX_NUMBER_OF_CHILDREN + 1];
static int read_pipes[MAX_NUMBER_OF_CHILDREN + 1][MAX_NUMBER_OF_CHILDREN + 1];

static const char * const  new_pipe_fmt = "Pipe r: %d w: %d (%d,%d)\n";

int childrenNumber;
int numberOfReceivedStartedMessages = 0;
int numberOfReceivedDoneMessages = 0;
int id;

int eventsLogDescriptor;
int pipesLogDescriptor;
char buf[64];

int receive(void * self, local_id from, Message * msg){
	int dest = (int)self;
	int nbytes = read(read_pipes[dest][from], msg, sizeof(*msg));
	if (nbytes > 0) {  /// ???
		return 0;
	}

	return -1;
}

int receive_any(void * self, Message * msg){
	int dest = (int)self;
	while (1) {
		for (int i = 1; i <= childrenNumber; i ++) {
			if (dest != i) {
				if (receive(self, i, msg) == 0) {
					return 0;
				}
                usleep(20000);
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
			int result = send(self, i, msg);
			if (result != 0) {
				return -1;
			}
		}
	}

	return 0;
}

void logWrite(int descriptor, char buffer[])
{
    write(descriptor, buffer, strlen(buffer));
    printf("%.*s",(int)strlen(buffer),buffer);
}

Message createMessage(int m_type, char buffer[]) {
	MessageHeader header = {MESSAGE_MAGIC, strlen(buffer), m_type, (int)time(NULL)};
    Message message;
    message.s_header = header;
    sprintf(message.s_payload, buffer, strlen(buffer));
    return message;
}

void started() {
	int pid = getpid();
	int parentProcessId = getppid();
	
	sprintf(buf, log_started_fmt, id, pid, parentProcessId);
    logWrite(eventsLogDescriptor, buf);

    Message message = createMessage(STARTED, buf);

    if (send_multicast(INT2VOIDP(id), &message) != 0) {
    	printf("send_multicast() failed");
    }
}

void receiveAllStartedMessages() {
    while (numberOfReceivedStartedMessages < childrenNumber - 1) {
		Message* msg = malloc(sizeof(*msg));
		if (receive_any(INT2VOIDP(id), msg) != 0) {
			printf("receive_any() failed");
		}

		if (msg[0].s_header.s_type == STARTED) {
			numberOfReceivedStartedMessages = numberOfReceivedStartedMessages + 1;
		} else if (msg[0].s_header.s_type == DONE) {
			numberOfReceivedDoneMessages = numberOfReceivedDoneMessages + 1;
		}
	}
}

void receivedAllStarted() {
    sprintf(buf, log_received_all_started_fmt, id);
    logWrite(eventsLogDescriptor, buf);
}

void done() {
    sprintf(buf, log_done_fmt, id);
    logWrite(eventsLogDescriptor, buf);

    Message message = createMessage(DONE, buf);

    if (send_multicast(INT2VOIDP(id), &message) != 0) {
    	printf("send_multicast() failed");
    }
}

void receiveAllDoneMessages() {
    while (numberOfReceivedDoneMessages < childrenNumber - 1) {
    	Message* msg = malloc(sizeof(*msg));
    	if (receive_any(INT2VOIDP(id), msg) != 0) {
    		printf("receive_any() failed");
    	}

    	if (msg[0].s_header.s_type == DONE) {
    		numberOfReceivedDoneMessages = numberOfReceivedDoneMessages + 1;
    	}
    }
}

void receivedAllDoneMessages() {
    sprintf(buf, log_received_all_done_fmt, id);
    logWrite(eventsLogDescriptor, buf);
}

void be_childish(int local_id)
{
	id = local_id;

    for (int i = 0; i <= childrenNumber; i ++) {
        for (int j = 0; j <= childrenNumber; j ++) {
            if (i != id && i != j) {
                close(read_pipes[i][j]);
                close(write_pipes[i][j]);
            }
        }
    }

	started();
    receiveAllStartedMessages();
    receivedAllStarted();
    done();
    receiveAllDoneMessages();
    receivedAllDoneMessages();

    exit(0);
}

void openLogFiles(){
	eventsLogDescriptor = open(events_log, O_WRONLY | O_APPEND | O_CREAT, 0666);
	pipesLogDescriptor = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT, 0666);
}

void prepare(){
    remove(events_log);
    remove(pipes_log);
    openLogFiles();
}

void createPipes() {
	int i, j;
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
		            return;
		        }
	    		read_pipes[j][i] = new_pipe[0];
	    		fcntl(read_pipes[j][i], F_SETFL, O_NONBLOCK);
	    		write_pipes[i][j] = new_pipe[1];
	            char buf[64];
	            sprintf(buf, new_pipe_fmt, j, i, new_pipe[0],new_pipe[1]);
	            write(pipesLogDescriptor, buf, strlen(buf));
	        }
    	}
    }
    close(pipesLogDescriptor);
}

void createChildren() {
	for (int i = 1; i <= childrenNumber; i ++)
    {
    	pid_t pid = fork();

    	if (pid < 0)
        {
            int errnum = errno;
            fprintf(stderr, "Fork failed (%d: %s)\n", errnum, strerror(errnum));
            return;
        }

        if (pid == 0)
        {
        	be_childish(i);
        } else {
        }
    }
    for (int i = 1; i <= childrenNumber; i ++) {
        for (int j = 0; j <= childrenNumber; j ++) {
            if (i != j) {
                close(read_pipes[i][j]);
                close(write_pipes[i][j]);
            }
        }
    }
    close(eventsLogDescriptor);
}

void receiveMessages() {
    while (numberOfReceivedStartedMessages < childrenNumber || numberOfReceivedDoneMessages < childrenNumber) {
    	Message* msg = malloc(sizeof(*msg));
    	if (receive_any(INT2VOIDP(0), msg) != 0) {
    		printf("receive_any() failed");
    	}

        // printf("%d received message: %s", 0, msg[0].s_payload); ///

    	if (msg[0].s_header.s_type == STARTED) {
    		numberOfReceivedStartedMessages = numberOfReceivedStartedMessages + 1;
    	} else if (msg[0].s_header.s_type == DONE) {
    		numberOfReceivedDoneMessages = numberOfReceivedDoneMessages + 1;
    	}
    }
}



int main(int argc, char **argv) {
	childrenNumber = atoi(argv[2]);

    prepare();
    createPipes();
    createChildren();
    receiveMessages();

    while(wait(NULL)>0) {
    }

    return 0;
 
}
