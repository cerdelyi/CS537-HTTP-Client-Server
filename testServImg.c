//
//  testServ.c
//  CS537_TCP_Server
//
//  Created by Christopher Erdelyi on 3/3/18.
//  Copyright © 2018 Christopher Erdelyi. All rights reserved.
//

//
//  caseServer.c
//  CS537_TCP_Server
//
//  Created by Christopher Erdelyi on 2/19/18.
//  Copyright © 2018 Christopher Erdelyi. All rights reserved.
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

/**
 WHAT IS NEEDED AND NECESSARY:
 be able to parse out to http 1.0 or 1.1
 
 function to return status codes
 function to read size of file to place in "Content-Length" header  <-- important. Will not work without this.
 function which sets line for "Content-Type" <--- need for images
 *learn how to read in files in C and determine their file size*
 
 
 */

/* void *headerParse(void *arg)
 {
 
 
 } */

void *headerReturn(char input[200])
{
    
    int value = 1223;
    char* test = "Testing";
    sprintf(input, "%d %s", value, test);
    return 0;
}




void *clientHandler(void *arg)
{
    
    char str[MAXLINE];
    char msg[MAXLINE];
    
    int i, n;
    
    strncpy(msg, "HTTP/1.1 200 OK \n Date: Mon, 27 Jul 2009 12:28:53 GMT \n Server: Apache \nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT \n Accept-Ranges: bytes \nContent-Length: 88 \nVary: Accept-Encoding\nContent-Type: text/html\r\n\r\n <html>\n <body>\n Hello World! My payload includes a trailing CRLF.\n </body>\n </html>", MAXLINE);
    
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
    
    char* buffer;
    size_t size = 10;
    
    //    int testLength = msgLength(data1);
    
    int fd = *(int*)(arg);
    char* string_tokens;
    //  char* getCommand = "GET";
    
    FILE* testHTML = fopen("test.html", "r");
    
    // char inputString[999];
    
    
    int ch;
    size_t len = 0;
    buffer = realloc(NULL, sizeof(char)*size);//size is start size
    
    
    
    while(EOF!=(ch=fgetc(testHTML))){
        buffer[len++]=ch;
        if(len==size){
            buffer = realloc(buffer, sizeof(char)*(size+=16));
        }
    }
    buffer[len++]='\0';
    
    
    buffer = realloc(buffer, sizeof(char)*len);
    
    fclose(testHTML);
  
    
    ////////////////////////////////////////////////
    // IMAGE HANDLING SECTION
   
    /*
    FILE* testGif = fopen("indyicon.jpg", "r");
    int gifsize;
    fseek(testGif, 0, SEEK_END);
    gifsize = ftell(testGif);
    fseek(testGif, 0, SEEK_SET);
    
    char* fullGifHeader = (char*) malloc(10+strlen(gifHeader)+gifsize);
    char* gifContentSize= (char*) malloc(gifsize);
    
   snprintf(gifContentSize, gifsize, "%d", gifsize);
    strcpy(fullGifHeader, gifHeader);
    strcat(fullGifHeader, gifContentSize);
    strcat(fullGifHeader, trailingNewline);
    
    //Send Picture as Byte Array
 
    char send_buffer[1];
    while(!feof(testGif)) {
        fread(send_buffer, 1, sizeof(send_buffer), testGif);
        strcat(fullGifHeader, send_buffer);
        //write(fd, send_buffer, sizeof(send_buffer));
        bzero(send_buffer, sizeof(send_buffer));
    } */
  

    
    
    FILE* file = fopen("indyicon.jpg", "rb");
    fseek(file, 0, SEEK_END);
    int fileLen=ftell(file);
    //char* file_data;
    // Try array for file_data
    char file_data[fileLen];
    rewind(file);
   /* file_data=malloc((fileLen)*sizeof(char));
    if (file_data == NULL){
        printf("Memory error"); exit (2);
    } */
    
    
    // TRY DIFFERENT METHOD OF READING FILE. NO CHAR STRINGS
  //   char imageBinFile[fileLen];
  //  fread(imageBinFile, 1, fileLen, file);
    
    //ADD HEADER INFO: 200 OK, CONTENT-LENGTH, TRAILING NEW LINE, IMAGE BYTES
    
    
   
    fread(file_data, sizeof(char), fileLen+1, file);
    printf("fileLen: %d \n", fileLen);
    printf("file_data: %s \n", file_data);
    
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
    
    while (1) {
        
       
        
        if ((n = read(fd, str, MAXLINE)) == 0) {
            write(fd, "closing connection", MAXLINE);
            close (fd);
            return 0;
        }
        
        
        int ContentHeaderSize = strlen(buffer);
        char Content_Header_Length[ContentHeaderSize];
        
        snprintf(Content_Header_Length, ContentHeaderSize, "%d", ContentHeaderSize);
        
        
        char* fullHeader = (char*) malloc(strlen(trailingNewline)+ strlen(data)+strlen(Content_Header_Length));
        strcpy(fullHeader, data);
        strcat(fullHeader, Content_Header_Length);
        strcat(fullHeader, trailingNewline);
        
        
        
        char* fullData = (char*) malloc(strlen(fullHeader)+ strlen(buffer));
        strcpy(fullData, fullHeader);
        strcat(fullData, buffer);
        
     
        
        string_tokens= strtok(str, " ");
        if (strncmp(string_tokens, "GET", 3)==0){
            
            printf("in the loop, saw a GET\n");
            string_tokens= strtok(NULL, " ");
            printf("Next token is: %s \n", string_tokens);
            
            
            if(strncmp(string_tokens, "/", 3)==0)
            {
                printf("in the loop, saw a slash\n");
               // printf("Full data: %s\n",fullData);
                write(fd, fullData, strlen(fullData)+1);
                
            }
        
            
            //TEST SECTION FOR IMAGE
            if (strncmp(string_tokens, "/indyicon.jpg", 12)==0)
            {
                printf("its indy time\n");
                write(fd, fullImgHeader, strlen(fullImgHeader));
              //  printf("sending fullImgHeader: %s \n", fullImgHeader);
                write(fd, file_data, fileLen);
              //  printf("sending file_data: %s \n", file_data);
                printf("Competed image writes \n");
               
            }
       
        
        }
        
        
        else if (strncmp(string_tokens, "PUT", 3)==0){
            write(fd, "404 error", MAXLINE);
        }
        
       /* else{
            write(fd, "skipped those checks", 256);
        } */
        
    //    free(buffer);
    //    free(fullHeader);
    //    free(fullData);
        
        
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

