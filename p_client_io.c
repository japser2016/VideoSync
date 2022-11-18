/* Sending and Receiving Functions from Client */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>

#include "p_client_io.h"

void error(char *msg) {
  perror(msg);
  exit(1);
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
    char *buf = malloc(BUFSIZE);
    
    int fd = open(vidLoc, O_RDONLY);
    do {
        
        bzero(buf,BUFSIZE);
        
        n = read(fd, buf, BUFSIZE);
        if (n < 0)
            error("failed to read file");
        printf("read %d bytes\n", n);
        printf("writing %d bytes to socket\n", n);

        m = write(sockfd, buf, n);
        printf("wrote %d bytes to socket\n",m);
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
    char filename[128] = "video";
    printf("filename %s\n",vid->filename);
    //strcat(filename,strchr(vid->filename, '.'));
    char ack[1];
    ack[0] = (char) 9;
    write(sockfd, ack, 1);
    
    int fd = open(filename, O_WRONLY|O_CREAT|O_SYNC|O_APPEND, S_IRUSR|S_IWUSR);
    char *buf = malloc(BUFSIZE);
    while (n > 0) {
        bzero(buf, BUFSIZE);

        n = read(sockfd, buf, BUFSIZE);
        
        if (n < 0)
            error("failed to read from socket");
        printf("read %d bytes from socket %d\n",n,BUFSIZE);        

        printf("writing %d bytes to %s\n", n, filename);
        m = write(fd, buf, n);
        printf("wrote %d bytes to file\n",m);
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
