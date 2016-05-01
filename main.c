#include <stdio.h>

#include "common.h"
#include "ipc.h"
#include "pa1.h"

int main(int argc, char *argv[]) {
	//printf(events_log);
	//printf("%s\n", log_received_all_done_fmt);
	printf("Count: %d\n", argc);
	printf("You entered: %s\n", argv[0]);
	printf("%s\n",argv[1]);
	getchar();
	return 0;
}		