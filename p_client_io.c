/* Sending and Receiving Functions from Client */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "p_client_io.h"

void error(char *msg) {
  perror(msg);
  exit(1);
}

void serve_videofile(struct sockaddr_in serveraddr, struct sockaddr_in clientaddr, char *vidLoc) {
    int parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0) 
        error("ERROR opening socket");

    int optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
               (const void *)&optval , sizeof(int));

    if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
        error("ERROR on binding");

    if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
        error("ERROR on listen");

    int clientlen = sizeof(clientaddr);
    int childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0) 
        error("ERROR on accept");
    send_video(childfd, vidLoc);
    close(childfd);
    close(parentfd);
}



int send_video(int sockfd, char *vidLoc) {

    
    struct VIDPKT *vid = malloc(sizeof(struct VIDPKT));
    bzero(vid,sizeof(struct VIDPKT));
    vid->type = (char)8;
    printf("filename len: %d\n", strlen(vidLoc));
    strcpy(vid->filename, vidLoc);
    printf("filename len: %d\n", strlen(vid->filename));
    printf("file: %s\n",vid->filename);
    int x = write(sockfd, vid, strlen(vid->filename) + 2);
    printf("wrote %d bytes for setting up video xfer\n",x);

    //wait for ack
    char ack[1];
    x = read(sockfd, ack, 1);
    if(ack[0] != (char) 9)
        return -1;
    
    int n,m = 0;
    char *buf = malloc(8192);
    
    int fd = open(vidLoc, O_RDONLY);
    do {
        
        bzero(buf,8192);
        
        n = read(fd, buf, 8192);
        if (n < 0)
            error("failed to read file");
        /* printf("read %d bytes\n", n); */
        /* printf("writing %d bytes to socket\n", n); */

        m = write(sockfd, buf, n);
        //printf("wrote %d bytes to socket\n",m);
        if (m < 0)
            error("failed to write to socket");
        
    } while(m > 0);
    free(buf);
    free(vid);
    close(fd);
    return 0;
}

int receive_video(int sockfd) {
    struct VIDPKT *vid = malloc(sizeof(struct VIDPKT));

    int n,m = 0;
    bzero(vid,sizeof(struct VIDPKT));

    n = read(sockfd, vid, sizeof(struct VIDPKT));
    if (n < 0)
        error("failed to read from socket");
    printf("read %d bytes from socket %d\n",n,sizeof(struct VIDPKT));

    if (vid->type != (char)8)
        return -1;
    char filename[128] = "video_player/video";
    printf("filename %s\n",vid->filename);
    //strcat(filename,strchr(vid->filename, '.'));
    char ack[1];
    ack[0] = (char) 9;
    write(sockfd, ack, 1);
    
    int fd = open(filename, O_WRONLY|O_CREAT|O_SYNC|O_APPEND, S_IRUSR|S_IWUSR);
    char *buf = malloc(8192);
    while (n > 0) {
        bzero(buf, 8192);

        n = read(sockfd, buf, 8192);
        
        if (n < 0)
            error("failed to read from socket");
        //printf("read %d bytes from socket %d\n",n,BUFSIZE);        

        //printf("writing %d bytes to %s\n", n, filename);
        m = write(fd, buf, n);
        //printf("wrote %d bytes to file\n",m);
        if (m < 0)
            error("failed to write to file");
    }
    free(vid);
    free(buf);
    close(fd);
}



/* int main(int argc, char *argv[]) { */
/*     if (argc != 2) */
/*         return 1; */

    
/*     //send_video(1,"/GPP_Drive/Personal/Media/SFW/Videos/duvet.mp4"); */
/*     return 0; */
/* } */
