/* Sending and Receiving Functions from Client */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>

#include "p_client_io.h"
#include "packet.h"

void error(char *msg) {
  perror(msg);
  exit(1);
}

int send_video(int sockfd, char *vidLoc) {
    int fd = open(vidLoc, O_RDONLY);
    
    struct VIDPKT *vid = malloc(sizeof(struct VIDPKT));
    bzero(vid,sizeof(struct VIDPKT));
    vid->type = 8;
    int n = 0;
    //do {
    
        n = read(fd, vid->data, BUFSIZE);
        if (n < 0)
            error("failed to read file");

        

        
    
    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    //send_video(1,"/GPP_Drive/Personal/Media/SFW/Videos/duvet.mp4");
    return 0;
}
