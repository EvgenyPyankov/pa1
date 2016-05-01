#include <stdio.h>


#include "common.h"
#include "ipc.h"
#include "pa1.h"

int childrenNumber = 5;

FILE* eventsLog;
FILE* pipesLog;

void openLogFiles(){
	eventsLog = fopen(events_log,"w+");
	pipesLog = fopen(pipes_log,"w+");
}

void closeLogFiles(){
	fclose(eventsLog);
	fclose(pipesLog);
}

int main() {
	const char *buf = "123456789";
	openLogFiles();
	fprintf(eventsLog, "Some text:%s", buf);
	closeLogFiles();
	getchar();
	return 0;
}		