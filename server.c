
//  server.c
//  CS537_TCP_Server
//
//  Group Members: Christopher Erdelyi, John Wu
//
//  Copyright © 2018 Christopher Erdelyi & John Wu. All rights reserved.


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

const int backlog = 4;

struct threadParams{
	int passedFd;
	int passedHTTP;
};


void *clientHandler(void *arg)
{
	char str[MAXLINE];
	int n;
	
	struct threadParams *passedParams = (struct threadParams*) arg;
	int passedVersion = passedParams->passedHTTP;
	
	const char* htmlHeader =
	" 200 OK\r\n"
	"Content-Type: text/html\r\n"
	"Accept-Ranges: bytes\r\n"
	"Content-Length: ";
	
	const char* jpgHeader =
	" 200 OK\r\n"
	"Content-Type: image/jpeg\r\n"
	"Accept-Ranges: bytes\r\n"
	"Content-Length: ";
	
	const char* fourzerofour =
	" 404 Not Found\r\n"
	"Content-Type: text/html\r\n"
	"Connection: close\r\n"
	"Content-Length: 52\r\n\r\n"
	"<html><body><b>404 File Not Found</b></body></html>";
	
	const char* fouronefive =
	"HTTP/1.1 415 Unsupported Media Type\r\n"
	"Content-Type: text/html\r\n"
	"Connection: close\r\n"
	"Content-Length: 60\r\n\r\n"
	"<html><body><b>415 Unsupported Media Type</b></body></html>";
	
	
	char httpHead[20];
	if(passedVersion == 10)
	{
		strcpy(httpHead, "HTTP/1.0");
	}
	if(passedVersion == 11)
	{
		strcpy(httpHead, "HTTP/1.1");
	}
	
	int fd = (passedParams->passedFd);
	
	while (1) {
		char* r_type;	//hold request type
		char* path;		//hold path target for request
		char* fileExtension;	//hold file extension of path target
		char strcopy[MAXLINE];	//hold copy before strtok for PUT request
		char* indexstr = "index.html";	//string for index filename
		
		if ((n = read(fd, str, MAXLINE)) == 0) {
			write(fd, "closing connection", 18);
			close (fd);
			return 0;
		}
		//create copy in case PUT
		strcpy(strcopy,str);
		//extract request type
		r_type = strtok(str, " ");
		//extract path
		char* slashpath = strtok(NULL, " ");
		//set path to "index.html" if request for default path
		if(strcmp(slashpath, "/") == 0)
			path = indexstr;
		else
			path = slashpath+1;
		//extract file extension
		fileExtension = path+strcspn(path, ".")+1;
		
		//respond to GET or HEAD request
		if (strcmp(r_type, "GET")==0 || strcmp(r_type, "HEAD")==0)
		{
			//check if file exists
			if(access(path, F_OK) < 0)	//doesn't exist
			{
				char* fourZeroFourError = (char*) malloc(strlen(httpHead)+(strlen(fourzerofour)));
				strcpy(fourZeroFourError, httpHead);
				strcat(fourZeroFourError, fourzerofour);
				write(fd, fourZeroFourError, strlen(fourZeroFourError)+1);
			}
			else	//does exist
			{
				//request is html file
				if(strcmp(fileExtension, "html")==0)
				{
					//open file and find file length
					FILE* file = fopen(path, "r");
					fseek(file, 0, SEEK_END);
					int fileLen=ftell(file);
					char file_data[fileLen];
					rewind(file);
					
					//read file data
					fread(file_data, sizeof(char), fileLen, file);
					fclose(file);
					
					//convert file length to string
					char Content_Header_Length[50];
					sprintf(Content_Header_Length, "%d\r\n\r\n", fileLen);
					
					//make header
					char* fullHeader = (char*) malloc(strlen(htmlHeader)+strlen(Content_Header_Length)+strlen(httpHead));
					strcpy(fullHeader, httpHead);
					strcat(fullHeader, htmlHeader);
					strcat(fullHeader, Content_Header_Length);
					
					//GET-> send header+body || HEAD-> send header
					if(strcmp(r_type, "GET")==0)
					{
						write(fd, fullHeader, strlen(fullHeader));
						write(fd, file_data, fileLen);
					}
					else if (strcmp(r_type, "HEAD")==0)
						write(fd, fullHeader, strlen(fullHeader));
				}
				//request is jpg file
				else if(strcmp(fileExtension, "jpg")==0)
				{
					FILE* file = fopen(path, "rb");
					fseek(file, 0, SEEK_END);
					int fileLen=ftell(file);
					char file_data[fileLen];
					rewind(file);
					
					fread(file_data, sizeof(char), fileLen+1, file);
					fclose(file);
					
					//CONVERT LENGTH OF IMAGE FILE TO TEXT FORMAT FOR HEADER
					char Img_Content_Header_Length[50];
					sprintf(Img_Content_Header_Length, "%d\r\n\r\n", fileLen);
					
					//PUT IT ALL TOGETHER
					char* fullImgHeader = malloc(strlen(Img_Content_Header_Length) + strlen(jpgHeader)+strlen(httpHead));
					strcpy(fullImgHeader, httpHead);
					strcat(fullImgHeader, jpgHeader);
					strcat(fullImgHeader, Img_Content_Header_Length);
					
					if(strcmp(r_type, "GET")==0) {
						write(fd, fullImgHeader, strlen(fullImgHeader));
						write(fd, file_data, fileLen);
					}
					else if (strcmp(r_type, "HEAD")==0)
						write(fd, fullImgHeader, strlen(fullImgHeader));
				}
				//file is not html or jpg
				else
				{
					if(passedVersion == 11)
						write(fd, fouronefive, strlen(fouronefive)+1);
					else
						write(fd, "HTTP/1.0 400 Bad Request\r\n", 26);
				}
			}
		}
		//respond to PUT request
		else if(strcmp(r_type, "PUT")==0)
		{
			//check if html file (other file types not supported)
			if(strcmp(fileExtension, "html")==0 && strstr(strcopy, "Content-Type: text/html") != NULL)
			{
				char puthead[150];
				//check if file exists
				if(access(path, F_OK ) < 0)	//doesn't exist
				{
					strcpy(puthead, "HTTP/1.1 201 Created\r\nContent-Location: ");
					strcat(puthead, path);
					strcat(puthead, "\r\n");
				}
				else	//does exist
				{
					strcpy(puthead, "HTTP/1.1 204 No Content\r\nContent-Location: ");
					strcat(puthead, path);
					strcat(puthead, "\r\n");
				}
				//get length
				char* lenloc = strstr(strcopy, "Content-Length: ");
				int bodylen = atoi(lenloc+16);
				//get body
				char* bodyloc = strstr(strcopy, "\r\n\r\n") + 4;
				//write file
				FILE* file = fopen(path, "w");
				if(file!=NULL)
				{
					fwrite(bodyloc, sizeof(char), bodylen, file);
				}
				fclose(file);
				//send response
				write(fd, puthead, strlen(puthead));
			}
			//file is not html
			else
			{
				write(fd, fouronefive, strlen(fouronefive)+1);
			}
		}
		//respond to DELETE request
		else if(strcmp(r_type, "DELETE")==0)
		{
			//check if file exists
			if(access(path, F_OK ) < 0)	//doesn't exist
			{
				char* fourZeroFourError = (char*) malloc(strlen(httpHead)+(strlen(fourzerofour)));
				strcpy(fourZeroFourError, httpHead);
				strcat(fourZeroFourError, fourzerofour);
				write(fd, fourZeroFourError, strlen(fourZeroFourError)+1);
			}
			else	//does exist
			{
				if (remove(path) == 0)
					write(fd, "HTTP/1.1 204 No Content\r\n", 25);
				else
					write(fd, "HTTP/1.1 403 Forbidden\r\n\r\n<p>DELETE error: remove() failed.</p>", 63);
			}
		}
		//not a GET, HEAD, PUT, DELETE request
		else
		{
			if(passedVersion == 11)
				write(fd, "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET, HEAD, PUT, DELETE\r\n", 64);
			else
				write(fd, "HTTP/1.0 400 Bad Request\r\n", 26);
		}
	}
}


int main(int argc, char *argv[])
{
	
	int	listenfd, httpVersion=0, connfd;
	pthread_t tid;
	int	 clilen;
	struct	 sockaddr_in cliaddr, servaddr;
	int convert = 0;
	struct threadParams *passedParams;
	
	bzero(&servaddr, sizeof(servaddr));
	
	if (argc == 1) {
		printf("Usage: caseServer <address> <Optional: HTML version number (1 for 1.0, or 11 for 1.1)> <Optional: port> \n");
		return -1;
	}
	
	if(argc == 2)
	{
		httpVersion = 10; //Default to HTML 1.0
		servaddr.sin_port = htons(8888);
		printf("No HTTP version or port number detected. HTTP version is 1.0, and port is 8888.\n");
	}
	
	if(argc == 3)
	{
		if ((convert = atoi(argv[2]) > 1024)) // have to convert from char to int for port check and http version check
		{
			servaddr.sin_port = htons(atoi(argv[2]));
			httpVersion = 10;
			printf("HTTP will default to version 1.0\n");
		}
		else if(atoi(argv[2]) < 20)
		{
			httpVersion = atoi(argv[2]);
			servaddr.sin_port = htons(8888);
			printf("Port will default to 8888\n");
		}
	}
	
	if(argc == 4)
	{
		servaddr.sin_port = htons(atoi(argv[3]));
		httpVersion = atoi(argv[2]);
		
	}
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1)
	{
		fprintf(stderr, "Error unable to create socket, errno = %d (%s) \n",
				errno, strerror(errno));
		return -1;
	}
	
	servaddr.sin_family		= AF_INET;
	servaddr.sin_addr.s_addr   = inet_addr(argv[1]);
	
	
	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		fprintf(stderr, "Error binding to socket, errno = %d (%s) \n",
				errno, strerror(errno));
		return -1;
		
	}
	
	if (listen(listenfd, backlog) == -1) {
		fprintf(stderr, "Error listening for connection request, errno = %d (%s) \n",
				errno, strerror(errno));
		return -1;
	}
	
	
	while (1) {
		
		
		
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		passedParams = malloc(sizeof(*passedParams));
		passedParams -> passedFd = connfd;
		passedParams -> passedHTTP = httpVersion;
		
		if ((connfd < 0 )) {
			if (errno == EINTR)
				continue;
			else {
				fprintf(stderr, "Error connection request refused, errno = %d (%s) \n",
						errno, strerror(errno));
			}
		}
		
		if (pthread_create(&tid, NULL, clientHandler, (void *)passedParams) != 0) {
			fprintf(stderr, "Error unable to create thread, errno = %d (%s) \n",
					errno, strerror(errno));
		}
		
	}
}

