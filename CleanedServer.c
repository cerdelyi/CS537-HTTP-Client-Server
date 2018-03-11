//
//  CleanedServer.c
//  CS537_TCP_Server
//
//  Created by Christopher Erdelyi on 3/7/18.
//  Copyright © 2018 Christopher Erdelyi. All rights reserved.
//
//

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


void *clientHandler(void *arg)
{
    
    char str[MAXLINE];
    
    int n;
    
    
    const char* data =
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "Accept-Ranges: bytes\n"
    "Content-Length: ";
    
    const char* jpgHeader =
    "HTTP/1.1 200 OK\n"
    "Content-Type: image/jpeg\n"
    "Accept-Ranges: bytes\n"
    "Content-Length: ";
    
    const char *trailingNewline = "\r\n\r\n";

    int fd = *(int*)(arg);
	
    while (1) {
		char* r_type;	//hold request type
		char* path;		//hold path target for request
		char* fileExtension;	//hold file extension of path target
		char* strcopy;	//hold copy before strtok for PUT request
		
        if ((n = read(fd, str, MAXLINE)) == 0) {
            write(fd, "closing connection", MAXLINE);
            close (fd);
            return 0;
        }
		//create copy in case PUT		
		strcpy(strcopy,str);
		//extract request type
		r_type = strtok(str, " ");
		//extract path
		path = strtok(NULL, " ");
		//set path to "index.html" if request for default path
		if(strcmp(path, "/") == 0)
		{
			path = (char*) malloc(sizeof(char)*11);
			strcpy(path, "index.html");
		}
		//extract file extension
		fileExtension = path+strcspn(path, ".")+1;
		
		//respond to GET or HEAD request
        if (strcmp(r_type, "GET")==0 || strcmp(r_type, "HEAD")==0)
        {
			//check if file exists
			if(access(path, F_OK) < 0)	//doesn't exist
			{
				write(fd, "HTTP/1.1 404 Not Found", MAXLINE);
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
					
					//add file length to header
					int ContentHeaderSize = fileLen;
					char Content_Header_Length[ContentHeaderSize];
					char* fullHeader = (char*) malloc(strlen(trailingNewline)+ strlen(data)+strlen(Content_Header_Length));
					char* fullData = (char*) malloc(strlen(fullHeader)+ strlen(file_data));
					
					snprintf(Content_Header_Length, ContentHeaderSize, "%d", ContentHeaderSize);
					
					//generate header and header+body
					strcpy(fullHeader, data);
					strcat(fullHeader, Content_Header_Length);
					strcat(fullHeader, trailingNewline);
					strcpy(fullData, fullHeader);
					strcat(fullData, file_data);
					
					//GET-> send header+body || HEAD-> send header
					if(strcmp(r_type, "GET")==0)
						write(fd, fullData, strlen(fullData)+1);
					else if (strcmp(r_type, "HEAD")==0)
						write(fd, fullHeader, strlen(fullHeader)+1);
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
					
					//CONVERT LENGTH OF IMAGE FILE TO TEXT FORMAT FOR HEADER
					char Img_Content_Header_Length[fileLen];
					sprintf(Img_Content_Header_Length, "%d", fileLen);
					
					//PUT IT ALL TOGETHER
					char* fullImgHeader = malloc(strlen(trailingNewline) + strlen(Img_Content_Header_Length) + strlen(jpgHeader));
					strcpy(fullImgHeader, jpgHeader);
					strcat(fullImgHeader, Img_Content_Header_Length);
					strcat(fullImgHeader, trailingNewline);
					// strcat(fullImgHeader, file_data);
					
					//printf("file contents: %s", file_data);
					fclose(file);
					
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
					write(fd, "HTTP/1.1 415 Unsupported Media Type", MAXLINE);
				}
			}
        }
		//respond to PUT request
        else if(strcmp(r_type, "PUT")==0)
        {
			//check if html file (other file types not supported)
			if(strcmp(fileExtension, "html")==0)
			{
				char* puthead;
				//check if file exists
				if(access(path, F_OK ) < 0)	//doesn't exist
				{
					strcpy(puthead, "HTTP/1.1 201 Created\nContent-Location: ");
					strcat(puthead, path);
				}
				else	//does exist
				{
					strcpy(puthead, "HTTP/1.1 204 No Content\nContent-Location: ");
					strcat(puthead, path);		
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
				
				write(fd, puthead, MAXLINE);
			}
			//file is not html
			else
			{
				write(fd, "HTTP/1.1 415 Unsupported Media Type", MAXLINE);
			}
        }
        //respond to DELETE request    
        if(strncmp(r_type, "DELETE", 6)==0)
        {
			//check if file exists
			if(access(path, F_OK ) < 0)	//doesn't exist
			{
				write(fd, "HTTP/1.1 404 Not Found", MAXLINE);
			}
			else	//does exist
			{
				if (remove(path) == 0)
					write(fd, "HTTP/1.1 204 No Content", MAXLINE);
				else
				{
					write(fd, "HTTP/1.1 403 Forbidden\r\n\r\n<p>DELETE error: remove() failed.</p>", MAXLINE);
				}
			}
        }
        //not a GET, HEAD, PUT, DELETE request
        else
		{
            write(fd, "HTTP/1.1 405 Method Not Allowed", MAXLINE);
        }
    }
}


int main(int argc, char *argv[])
{
    
    int    listenfd, httpVersion, connfd;
    pthread_t tid;
    int     clilen;
    struct     sockaddr_in cliaddr, servaddr;
    int convert = 0;
    
    bzero(&servaddr, sizeof(servaddr));
    
   if (argc == 1) {
        printf("Usage: caseServer <address> <Optional: HTML version number (1 for 1.0, or 11 for 1.1)> <Optional: port> \n");
        return -1;
    }
    
    if(argc == 2)
    {
        httpVersion = 1; //Default to HTML 1.0
        servaddr.sin_port = htons(8888);
        printf("No HTTP version or port number detected. HTTP version is 1.0, and port is 8888.\n");
    }
    
    if(argc == 3)
    {
        if ((convert = atoi(argv[2]) > 1024)) // have to convert from char to int for port check and http version check
        {
            servaddr.sin_port = htons(atoi(argv[2]));
            httpVersion = 1;
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
        httpVersion = htons(atoi(argv[2]));
        
    }
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        fprintf(stderr, "Error unable to create socket, errno = %d (%s) \n",
                errno, strerror(errno));
        return -1;
    }
    
    servaddr.sin_family        = AF_INET;
    servaddr.sin_addr.s_addr   = inet_addr(argv[1]);
    //servaddr.sin_port          = htons(atoi(argv[3]));
    
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
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0 ) {
            if (errno == EINTR)
                continue;
            else {
                fprintf(stderr, "Error connection request refused, errno = %d (%s) \n",
                        errno, strerror(errno));
            }
        }
        
        if (pthread_create(&tid, NULL, clientHandler, (void *)&connfd) != 0) {
            fprintf(stderr, "Error unable to create thread, errno = %d (%s) \n",
                    errno, strerror(errno));
        }
        
    }
}

