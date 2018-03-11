#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MAXLINE 2048
#define CHUNK 1

int main () {
	char str[MAXLINE] = "PUT / HTTP/1.1\n"
		"Content-Type: text/html\n"
		"Accept-Ranges: bytes\n"
		"Content-Length: 109\r\n\r\n"		
		"<html>\n"
		"<head>\n"
		"<title>PUT!</title>\n"
		"</head>\n"
		"<body>\n"
		"Good job with that PUT!!!!!!!!!\n"
		"<br>\n"
		"</body>";
	
	char* request_type;
	char* path;
	char* file_ext;
	char* len;
	int n =0;
	
	request_type = strtok(str, "\n");
	while (request_type !=NULL)
	{
		printf("%d: %s\n", n, request_type);
		if (strncmp(request_type, "Content-Length: ", 16) == 0)
		{
			strcpy(len, request_type+16);
			printf("YAY: %s\n", len);
		}
		request_type = strtok(NULL, "\n");
		n = n+1;
	}

	return 0;
}
