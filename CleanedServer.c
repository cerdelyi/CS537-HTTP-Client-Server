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
    

    size_t size = 10;
    

    
    int fd = *(int*)(arg);
    char* string_tokens;
    char fileExtension[6];
    
    
    while (1) {
        
        if ((n = read(fd, str, MAXLINE)) == 0) {
            write(fd, "closing connection", MAXLINE);
            close (fd);
            return 0;
        }
        
        
        string_tokens= strtok(str, " ");
        if (strncmp(string_tokens, "GET", 3)==0)
        {
            char* fileExtensionSearch = string_tokens;
            
          
                string_tokens= strtok(NULL, " ");
                if((fileExtensionSearch = strrchr(string_tokens, '.')) != NULL)
                   {
                       if(strcmp(fileExtensionSearch, ".jpg")==0)
                       {
                           strcpy(fileExtension, "jpg");
                       }
                       
                       if(strcmp(fileExtensionSearch, ".html")==0)
                       {
                           strcpy(fileExtension, "html");
                       }
                   }
            
                 if((fileExtensionSearch = strrchr(string_tokens, '.')) == NULL) // Default case for browser sending GET / HTTP/1.X
                 {
                     strcpy(fileExtension, "html");
                 }
            
            if(strcmp(fileExtension, "html")==0)
            {
                //Check for HTML file request
                //if(strncmp(string_tokens, "/", 3)==0)    //Default case. Browser is asking for index.html
               // {
                    char* string_copy = string_tokens + 1;
                
                    char* fileToOpen;
                
                    if(strncmp(string_tokens, "/",3)==0)     //Default case. Browser is asking for index.html
                    {
                        fileToOpen = (char*) malloc(11);
                        strcpy(fileToOpen, "index.html");
                    }
                    else
                    {
                        fileToOpen = (char*) malloc(strlen(string_tokens));
                        strcpy(fileToOpen, string_copy);
                    }
                  
                    FILE* file = fopen(fileToOpen, "r");
                    fseek(file, 0, SEEK_END);
                    int fileLen=ftell(file);
                    char file_data[fileLen];
                    rewind(file);
                    
                    fread(file_data, sizeof(char), fileLen, file);
    
                    int ContentHeaderSize = fileLen;
                    char Content_Header_Length[ContentHeaderSize];
                    char* fullHeader = (char*) malloc(strlen(trailingNewline)+ strlen(data)+strlen(Content_Header_Length));
                    char* fullData = (char*) malloc(strlen(fullHeader)+ strlen(file_data));
                    
                    snprintf(Content_Header_Length, ContentHeaderSize, "%d", ContentHeaderSize);
                    
                    strcpy(fullHeader, data);
                    strcat(fullHeader, Content_Header_Length);
                    strcat(fullHeader, trailingNewline);
                    strcpy(fullData, fullHeader);
                    strcat(fullData, file_data);
                    
                    write(fd, fullData, strlen(fullData)+1);
               // }
            }
            
            if(strcmp(fileExtension, "jpg")==0)
            {
                char* string_copy = string_tokens + 1;
                //Check for image file request
                
                    char* fileToOpen = (char*) malloc(strlen(string_copy));
                    strcpy(fileToOpen, string_copy);
                    
                    FILE* file = fopen(fileToOpen, "rb");
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
                    
                    write(fd, fullImgHeader, strlen(fullImgHeader));
                    write(fd, file_data, fileLen);
                }
            }
        }
        
        if(strncmp(string_tokens, "PUT", 3)==0)
        {
            
        }
        
        if(strncmp(string_tokens, "HEAD", 4)==0)
        {
            
            
        }
        
        if(strncmp(string_tokens, "DELETE", 6)==0)
        {
            
            
        }
        
        else {      // SEND 404 ERROR
            write(fd, "404 error", MAXLINE);
        }
    }


int main(int argc, char *argv[])
{
    
    int    listenfd, connfd;
    pthread_t tid;
    int     clilen;
    struct     sockaddr_in cliaddr, servaddr;
    
    if (argc != 3) {
        printf("Usage: caseServer <address> <port> \n");
        return -1;
    }
    
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        fprintf(stderr, "Error unable to create socket, errno = %d (%s) \n",
                errno, strerror(errno));
        return -1;
    }
    
    bzero(&servaddr, sizeof(servaddr));
    
    servaddr.sin_family        = AF_INET;
    servaddr.sin_addr.s_addr   = inet_addr(argv[1]);
    servaddr.sin_port          = htons(atoi(argv[2]));
    
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

