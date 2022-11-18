/* Sending and Receiving Functions from Client */

#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>

#include "p_client_io.h"

int send_video(int sockfd, char *vidLoc) {
    int fd = open(vidLoc, "r");

    fseek(fd, 0L, SEEK_END);
    int sz = ftell(fd);
    rewind(fd);

    printf("File size: %d\n",sz);
    
    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    send_video(1,"/home/lord_gabem/Videos/duvet.webm");
}
