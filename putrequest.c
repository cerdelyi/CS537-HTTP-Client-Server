#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 256

void str_client(FILE *fp, int socket_fd);

int main(int argc, char *argv[])
{
	int	socket_fd;
	struct  sockaddr_in servaddr;

	if (argc != 3) {
		printf("Usage: caseClient <address> <port> \n");
        return -1;
	}

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
        fprintf(stderr, "Error creating socket, errno = %d (%s) \n", 
                errno, strerror(errno));
        return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	servaddr.sin_port = htons(atoi(argv[2]));

	if (connect(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        fprintf(stderr, "Error unable to connect to socket, errno = %d (%s) \n", errno,
                strerror(errno));
        return -1;
	}

	str_client(stdin, socket_fd);

	return 0;

}

void str_client(FILE *fp, int socket_fd)
{
	char	sndLine[MAXLINE];
	char	rcvLine[MAXLINE];

    memset((void *)sndLine, 0, MAXLINE);
    memset((void *)rcvLine, 0, MAXLINE);

    const char* putheader =
    "PUT new.html HTTP/1.1\n"
    "Content-Type: text/html\n"
    "Accept-Ranges: bytes\n"
    "Content-Length: ";
	
	FILE* file = fopen("old.html", "r");
    fseek(file, 0, SEEK_END);
    int fileLen=ftell(file);
    char* file_data;
    rewind(file);

	char clen[20];
	sprintf(clen, "%d\r\n\r\n", fileLen);
	
    file_data= (char*) malloc(fileLen);
    if (file_data == NULL){
        printf("Memory error"); exit (2);
    }
	fread(file_data, sizeof(char), fileLen, file);

    char* put_request= (char*) malloc((strlen(putheader)+strlen(clen)+fileLen)*sizeof(char));
	strcpy(put_request, putheader);
	strcat(put_request, clen);
	strcat(put_request, file_data);
	
	write(socket_fd, (void *)put_request, strlen(put_request));
	
	free(file_data);
	free(put_request);

    if (read(socket_fd, rcvLine, MAXLINE) == 0) {
		   printf("ERROR: server terminated \n");
		   return;
    }

    fputs(rcvLine, stdout);

}

	
