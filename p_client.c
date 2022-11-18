/* 
 * usage for first client: p_client <port> <video file location>
 * usage for other clients: p_client <port> <des_hostname> <des_port> <need_file 0/1>
 */
 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <errno.h>

#define BUFSIZE 1024
#define ARRAYSIZE 32
// each struct sockaddr_in has size 16 bytes -> 32*16 = 512 bytes
#define test_flag 1
#define REPEAT 2
#define TIMEOUT 2  //for select timeout in seconds


/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

/* 
 * main functions 
 */
void check_inputs(int argc, char **argv);
void read_inputs(int argc, char **argv, int *portno, char *vidLoc, char *des_hostname, int *des_portno, int *need_file);
int creat_socket();
int set_reuse_address(int parentfd);
int set_timeout_address(int parentfd, int enable, int sec);
void my_server_address_build(int portno, struct sockaddr_in *serveraddr);
void socket_port_bind(int parentfd, struct sockaddr_in *serveraddr);
void update_maxSocket(int* maxSocket, int currentSocket);
void prepare_select(fd_set *main_set, int* maxSocket, int parentfd);
struct sockaddr_in target_server_address_build(char *des_hostname, int des_portno, struct sockaddr_in *targetaddr);
int store_one_address(struct sockaddr_in client_address, struct sockaddr_in *client_list, int *client_list_counter);
int store_address_list(struct sockaddr_in *out_client_address_list, struct sockaddr_in *client_list, int *client_list_counter);
void main_loop(fd_set *main_set, int *maxSocket, int parentfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in *client_list, int *client_list_counter);
int state_signal_is_same(int play_state, int play_signal);
int store_timestamp(struct timeval *time_stamp);
int check_msg_type(char *buf, int msg_len);

/* functions handle msg received */
void handle_initial_hello(struct sockaddr_in *client_list, int *client_list_counter, struct sockaddr_in clientaddr);
void handle_userlist(struct sockaddr_in *client_list, int *client_list_counter);
void handle_pause(int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in client);
void handle_play(int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in client);

/* send functions */
int sends(int sockfd, char *buf, int buf_len, struct sockaddr_in targetaddr);
int send_to_all(int sockfd, char *buf, int buf_len, struct sockaddr_in *client_list, int client_list_counter, int repeat_time);
int construct_inital_hello(char *buf, int need_file);
int construct_pause(char *buf, struct timeval time_stamp);
int construct_pause_ack(char *buf);
int construct_play(char *buf, struct timeval time_stamp);
int construct_play_ack(char *buf);
int construct_userlist(char *buf, struct sockaddr_in *client_list, int client_list_counter);
int construct_userlist_ack(char *buf);

/* receive functions */
int receives(int sockfd, char *buf, int buf_len, struct sockaddr_in *serveraddr, int repeat_time);

/*
 * helper functions
 */
void showS(char* name, char* arg);
void showD(char* name, int arg);
void showSep();
void showPass(int i);
void showFail(int i);
void showAddress(struct sockaddr_in targetaddr);
int address_is_same(struct sockaddr_in s1, struct sockaddr_in s2);
int client_is_new(struct sockaddr_in client_address, struct sockaddr_in *client_list, int client_list_counter);
int timeS_is_same(struct timeval t1, struct timeval t2);
 
/*
 * testing funcitons
 */
void _test_read_inputs(int portno, char *vidLoc, char *des_hostname, int des_portno, int need_file);
void _test_parentfd_serverAddress_no_bind(int parentfd, struct sockaddr_in serveraddr);
void _test_prepare_select(fd_set main_set, int maxSocket, int parentfd);
void _test_target_server_address_build(struct sockaddr_in targetaddr);
void _test_address_is_same(struct sockaddr_in s1, struct sockaddr_in s2);
void _test_client_is_new(struct sockaddr_in s1, struct sockaddr_in s2);
void _test_store_one_address(struct sockaddr_in s1, struct sockaddr_in s2);
void _test_construct_inital_hello();
//void _test_sends();
void _test_state_signal_is_same();
void _test_construct_pause();
void _test_construct_play();
void _test_send_to_all(int sockfd, struct sockaddr_in *client_list, int client_list_counter);
void _test_construct_userlist();



/*****************************************************************/
int main(int argc, char **argv){
	/* initial set */
	int portno, des_portno; /* port to listen on OR to connect to */
	int need_file = 0; /* flag to show if need file or not (0/1)*/
	char vidLoc[BUFSIZE]; /* video file location */ 
	char des_hostname[BUFSIZE]; /* hostname to connect to */ 
	int parentfd;  /* socket fd for my UDP socket */
	struct sockaddr_in serveraddr; /* server's addr (myself) */
	struct sockaddr_in targetaddr; /* target server's addr (parent) */
	fd_set main_set;   /* , temp_set;  fd_set of all sockets */
	int maxSocket = 0; /* the max socket fd number */
	int len; /* store returned length for diff functions */
	char buf[BUFSIZE];
	
	/* logic set */
	int play_state = 0; /* 0 for pause, 1 for play */
	int play_signal = 0; /* receive play/pause signal from browser */
	struct timeval time_stamp; /* time stamp show watching progress */	
	time_stamp.tv_sec = 0;
	time_stamp.tv_usec = 0;
	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in)); /* array to store clients */
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0; /* current client array counter */
	
	/* helper variables */
	int result;
	
	/* load inputs */
	check_inputs(argc, argv);
	read_inputs(argc, argv, &portno, vidLoc, des_hostname, &des_portno, &need_file); 
	_test_read_inputs(portno, vidLoc, des_hostname, des_portno, need_file);	
	
	/* set socket & server address & bind port*/
	parentfd = creat_socket();	
	set_reuse_address(parentfd);
	my_server_address_build(portno, &serveraddr);
	socket_port_bind(parentfd, &serveraddr);
	_test_parentfd_serverAddress_no_bind(parentfd, serveraddr);
	
	/* prepare for select() */
	prepare_select(&main_set, &maxSocket, parentfd);
	_test_prepare_select(main_set, maxSocket, parentfd);
	
	/* if I am a new client (not the first main client) */
	if (argc == 5){
		/* setup target server + store target server */
		target_server_address_build(des_hostname, des_portno, &targetaddr);
		_test_target_server_address_build(targetaddr);
		_test_address_is_same(serveraddr, targetaddr);
		_test_client_is_new(serveraddr, targetaddr);
		_test_store_one_address(serveraddr, targetaddr);
		result = store_one_address(targetaddr, client_list, &client_list_counter);			
		
		_test_construct_inital_hello();
		len = construct_inital_hello(buf, need_file);
		/* send INITIAL HELLO - REPEAT times */
		for (int i = 0; i < REPEAT; i++){
			sends(parentfd, buf, len, targetaddr);
		}
	}
	main_loop(&main_set, &maxSocket, parentfd, &play_state, &play_signal, &time_stamp, client_list, &client_list_counter);
	
	
	free(client_list);
	client_list_counter = 0;
	
	return 0;
}




/*****************************************************************/


/************************************************/
/**             main functions                  */
/************************************************/

/* 
 * check the format of command line inputs
 * raise error if sth. wrong
 */
void check_inputs(int argc, char **argv){
	if (argc != 3 && argc != 5) {
		fprintf(stderr, "usage: %s <port> <video file location> OR %s <port> <des_hostname> <des_port> <need_file 0/1>\n", argv[0], argv[0]);
		exit(1);
	}	
	if (strlen(argv[2]) >= BUFSIZE){
		fprintf(stderr, "<video file location>/<des_hostname>: too long\n");
		exit(1);
	}
	if (argc == 5){
		char c1 = '0';
		char c2 = '1';
		if ((*(argv[4]) != c1 && *(argv[4]) != c2) || strlen(argv[4]) != 1){
			fprintf(stderr, "<need_file 0/1>: should be either 0 or 1\n");
			exit(1);
		}
	}
}

/*
 * store command line inputs to variables
 */
void read_inputs(int argc, char **argv, int *portno, char *vidLoc, char *des_hostname, int *des_portno, int *need_file){
	*portno = atoi(argv[1]);
	if (argc == 3){
		strcpy(vidLoc, argv[2]);
	} else {
		strcpy(des_hostname, argv[2]);
		*des_portno = atoi(argv[3]);	
		*need_file = atoi(argv[4]);	
	}
}

/*
 * creat a new UDP socket
 */
int creat_socket(){
	int new_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (new_socket < 0) 
		error("ERROR opening socket");
	return new_socket;
}

/*
 * set the socket to be reusable for debugging
 */
int set_reuse_address(int parentfd){
	int optval = 1;
	int result = setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
				(const void *)&optval , sizeof(int));
	if (result != 0){
		printf("setsockopt for address reuse NOT success\n");
	}
	return result;
}

/*
 * enable(enable=1) or disable(enable=0) socket's time out
 * timeout duration is set by sec 
 */
int set_timeout_address(int parentfd, int enable, int sec){
	return 0;
}

/*
 * build server's internet address 
 * and store in the address structure (sockaddr_in) passed in
 */
void my_server_address_build(int portno, struct sockaddr_in *serveraddr){
	bzero((char *) serveraddr, sizeof(*serveraddr));
	serveraddr->sin_family = AF_INET;
	serveraddr->sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr->sin_port = htons((unsigned short)portno);
}

/*
 * bind socket (parentfd) with the port (portno)
 */
void socket_port_bind(int parentfd, struct sockaddr_in *serveraddr){
	if (bind(parentfd, (struct sockaddr *) serveraddr, sizeof(*serveraddr)) < 0) 
		error("ERROR on binding");	
}

/*
 * set maxSocket to be the larger one (compared to currentSocket)
 */
void update_maxSocket(int* maxSocket, int currentSocket){
	if (*maxSocket < currentSocket){
		*maxSocket = currentSocket;
	}
}

/*
 * prepare fd_sets (main_set) used by select():
 * set main_set to empty, and put parentfd into main_set
 * and call updata_maxSocket to update maxSocket
 */
void prepare_select(fd_set *main_set, int* maxSocket, int parentfd){
	FD_ZERO(main_set);
	//FD_ZERO(temp_set);
	update_maxSocket(maxSocket, parentfd);
	FD_SET(parentfd, main_set); 
}

/*
 * build target server's internet address (to send msg to)
 * and return the address structure (sockaddr_in)
 */
struct sockaddr_in target_server_address_build(char *des_hostname, int des_portno, struct sockaddr_in *targetaddr){
	struct hostent *server = gethostbyname(des_hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", des_hostname);
        exit(0);
    }
	
	bzero((char *) targetaddr, sizeof(*targetaddr));
	targetaddr->sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		  (char *)&targetaddr->sin_addr.s_addr, server->h_length);
	targetaddr->sin_port = htons(des_portno);
	return *targetaddr;
}

/*
 * check if client_address (sockaddr_in) existing in the client_list
 * by calling client_is_new()
 * if not, store it to the list
 * return 1 if success, 0 if not success, -1 if ARRAYSIZE not enough
 */
int store_one_address(struct sockaddr_in client_address, struct sockaddr_in *client_list, int *client_list_counter){
	if (client_is_new(client_address, client_list, *client_list_counter)){
		if (*client_list_counter < ARRAYSIZE){
			client_list[(*client_list_counter)++] = client_address;
			return 1;
		}
		printf("store_one_address: add client failed -> ARRAYSIZE %d is not enough\n", ARRAYSIZE);
		return -1;
	}
	printf("store_one_address: client existed\n", ARRAYSIZE);
	return 0;
}

/*
 * store the whole client list provided (out_client_address_list) 
 * to our local list (append)
 */
int store_address_list(struct sockaddr_in *out_client_address_list, struct sockaddr_in *client_list, int *client_list_counter){
	return 0;
}

/*
 * the main loop to handle 
 * user's commands (play/pause)
 * msg received from other peers 
 * eg. initial hello, user list, pause/play signal
 */
void main_loop(fd_set *main_set, int *maxSocket, int parentfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in *client_list, int *client_list_counter){
	/* tests */
	_test_state_signal_is_same();
	_test_construct_pause();
	_test_construct_play();
	//_test_send_to_all(parentfd, client_list, *client_list_counter);
	_test_construct_userlist();
	/*******************************************/
	
	fd_set temp_set;
	int readyNo = 0;
	while(1){
		//_test_send_to_all(parentfd, client_list, *client_list_counter);
		int same = state_signal_is_same(*play_state, *play_signal);
		//*play_state = 1;
		*play_signal = 1;
		if (same == 0){
			if (*play_signal == 0){
				/* set play state */
				*play_state = 0;
				/* store/change timestamp */
				store_timestamp(time_stamp);
				/* construct and send PAUSE */
				char buf[BUFSIZE];
				bzero(buf, BUFSIZE);
				int buf_len = construct_pause(buf, *time_stamp);
				send_to_all(parentfd, buf, buf_len, client_list, *client_list_counter, REPEAT);
				
			} else {
				/* store/change timestamp */
				store_timestamp(time_stamp);
				/* construct and send PLAY */
				char buf[BUFSIZE];
				bzero(buf, BUFSIZE);
				int buf_len = construct_play(buf, *time_stamp);
				send_to_all(parentfd, buf, buf_len, client_list, *client_list_counter, REPEAT);
				/* set play state */
				*play_state = 1;
			}
		}
		
		temp_set = *main_set;
		/* set select() time out interval to TIMEOUT seconds */
		struct timeval timeout;
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		printf("before select\n");
		readyNo = select(*maxSocket+1, &temp_set, NULL, NULL, &timeout);
		printf("after select\n");
		if (readyNo < 0){
			fprintf(stderr, "ERROR in select(): %d\n", errno);
			break;
		} else if (readyNo > 0) {
			/* receive the message */
			char buf[BUFSIZE];
			bzero(buf, BUFSIZE);
			struct sockaddr_in clientaddr;
			int clientlen = sizeof(clientaddr);
			bzero((char *) &clientaddr, sizeof(clientaddr));
			int len = recvfrom(parentfd, buf, BUFSIZE, 0,
					(struct sockaddr *) &clientaddr, &clientlen);
			int msg_type = check_msg_type(buf, len);
			/* show msg for debugging */
			showSep();
			showD("len(received)", len);
			showS("buf", buf);
			showD("msgType", msg_type);	
			/* handle messages */
			switch(msg_type){
				case 1:
					/* if INITIAL HELLO */
					handle_initial_hello(client_list, client_list_counter, clientaddr);
			}
			
			//deal_clients(parentfd, main_set, &temp_set, maxSocket, readyNo, client_list, &client_list_counter);
			
			
			/* test INITIAL HELLO 
			if (len != 2 || buf[0] != 1 || buf[1] != 1){
				showFail(1);
			}
			*/
			
			
			
		}
		
	}
	
}

/*
 * check if play_state and play_signal are different
 * same return 1, diff return 0
 */
int state_signal_is_same(int play_state, int play_signal){
	if (play_state == play_signal){
		return 1;
	}
	return 0;
}

/*
 * store the current time stamp from the video (local)
 */
int store_timestamp(struct timeval *time_stamp){
	return 1;
}

/*
 * return the msg type received
 * return -1 if error
 */
int check_msg_type(char *buf, int msg_len){
	if (msg_len >= 1){
		return (int)(buf[0]);
	}
	return -1;
}

/* functions handle msg received */

/*
 * NOT send HELLO ACK 
 * send client list
 * store new client
 */
void handle_initial_hello(struct sockaddr_in *client_list, int *client_list_counter, struct sockaddr_in clientaddr){
	
}

/*
 * send USER LIST ACK
 * store user list if local list is new (with only one peer)
 */
void handle_userlist(struct sockaddr_in *client_list, int *client_list_counter){
	
}

/*
 * send PAUSE ACK
 * set the time stamp (according to the message)
 * change play state and signal if diff
 */
void handle_pause(int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in client){
	
}

/*
 * send PLAY ACK
 * set the time stamp (according to the message)
 * change play state and signal if diff
 */
void handle_play(int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in client){
	
}

/* send functions */
int sends(int sockfd, char *buf, int buf_len, struct sockaddr_in targetaddr){
	int serverlen = sizeof(targetaddr);
	/*
	showD("sockfd", sockfd);
	showD("buf_len", buf_len);
	showD("serverlen", serverlen);
	showAddress(targetaddr);
	*/
	int len = sendto(sockfd, buf, buf_len, 0, (struct sockaddr *) &targetaddr, serverlen);	
    if (len < 0) 
      error("ERROR in sendto");
	return len;
}
/* send the buf to all the client in the client list (with repeat)*/
int send_to_all(int sockfd, char *buf, int buf_len, struct sockaddr_in *client_list, int client_list_counter, int repeat_time){
	//check if store add the counter
	int i = 0;
	for (i = 0; i < client_list_counter; i++){
		for (int j = 0; j < repeat_time; j++){
			int len = sends(sockfd, buf, buf_len, client_list[i]);
		}
	}
}
int construct_inital_hello(char *buf, int need_file){
	bzero(buf, BUFSIZE);
	int len = 0;
	buf[len++] = (char)1;
	/*
	memcpy(&(buf[len]), &my_address, sizeof(my_address));
	len += sizeof(my_address);
	*/
	buf[len++] = (char)need_file;
	return len;
}
int construct_pause(char *buf, struct timeval time_stamp){
	bzero(buf, BUFSIZE);
	int len = 0;
	buf[len++] = (char)3;
	memcpy(&(buf[len]), &time_stamp, sizeof(time_stamp));
	len += sizeof(time_stamp);
	return len;
}
int construct_pause_ack(char *buf){
	return 0;
}
int construct_play(char *buf, struct timeval time_stamp){
	bzero(buf, BUFSIZE);
	int len = 0;
	buf[len++] = (char)4;
	memcpy(&(buf[len]), &time_stamp, sizeof(time_stamp));
	len += sizeof(time_stamp);
	return len;
}
int construct_play_ack(char *buf){
	return 0;
}
int construct_userlist(char *buf, struct sockaddr_in *client_list, int client_list_counter){
	// check if buf size >= list size
	bzero(buf, BUFSIZE);
	int buf_len = 0;
	/* add msg type */
	buf[buf_len++] = (char)2;
	int copy_size = client_list_counter*sizeof(struct sockaddr_in);
	memcpy(&(buf[buf_len]), client_list, copy_size);
	buf_len = buf_len + copy_size;
	return buf_len;
}
int construct_userlist_ack(char *buf){
	return 0;
}

/* receive functions */
int receives(int sockfd, char *buf, int buf_len, struct sockaddr_in *serveraddr, int repeat_time){
	return 0;
}

/************************************************/
/**             helper functions                */
/************************************************/
void showS(char* name, char* arg){
	printf("%s->%s\n", name, arg);
}
void showD(char* name, int arg){
	printf("%s->%d\n", name, arg);
}
void showSep(){
	printf("---------------------------------\n");
}
void showPass(int i){
	printf("PASS-%d\n", i);
}
void showFail(int i){
	printf("FAIL-%d\n", i);
}
void showAddress(struct sockaddr_in targetaddr){
	char *hostaddrp = inet_ntoa(targetaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    showS("address", hostaddrp);
	showD("port", ntohs(targetaddr.sin_port));
}
/* return 1 for same, 0 for diff */
int address_is_same(struct sockaddr_in s1, struct sockaddr_in s2){
	/*
	char *hostaddrp = inet_ntoa(s1.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    showS("s1", hostaddrp);
    showD("s1 port", ntohs(s1.sin_port));
	char *hostaddrp2 = inet_ntoa(s2.sin_addr);
	if (hostaddrp2 == NULL)
      error("ERROR on inet_ntoa\n");
    showS("s2", hostaddrp2);
    showD("s2 port", ntohs(s2.sin_port));
	*/
	int result_sin_addr = memcmp(&(s1.sin_addr.s_addr), &(s2.sin_addr.s_addr), sizeof(s1.sin_addr.s_addr));
	int result_sin_port = memcmp(&(s1.sin_port), &(s2.sin_port), sizeof(s1.sin_port));
	if (result_sin_addr == 0 && result_sin_port == 0){
		return 1;
	}
	return 0;
}
int client_is_new(struct sockaddr_in client_address, struct sockaddr_in *client_list, int client_list_counter){
	for (int i = 0; i < client_list_counter; i++){
		if (address_is_same(client_address, client_list[i]) == 1){
			return 0;
		}
	}
	return 1;
}
/* compare time_stamps, return 1 if same, 0 if diff */
int timeS_is_same(struct timeval t1, struct timeval t2){
	if (t1.tv_sec == t2.tv_sec && t1.tv_usec == t2.tv_usec){
		return 1;
	}
	return 0;
}
 
/************************************************/
/**             testing functions               */
/************************************************/
void _test_read_inputs(int portno, char *vidLoc, char *des_hostname, int des_portno, int need_file){
	if (!test_flag)
		return;
	showSep();
	printf("_test_read_inputs:\n");	
	showD("portno",portno);
	showS("vidLoc",vidLoc);
	showS("des_hostname",des_hostname);
	showD("des_portno",des_portno);
	showD("need_file",need_file);
}
void _test_parentfd_serverAddress_no_bind(int parentfd, struct sockaddr_in serveraddr){
	if (!test_flag)
		return;
	showSep();
	printf("_test_parentfd_serverAddress_no_bind:\n");

	showD("parentfd",parentfd);
	char *hostaddrp = inet_ntoa(serveraddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    showS("serveraddr.sin_addr", hostaddrp);
	showD("serveraddr.sin_port", ntohs(serveraddr.sin_port));
}
void _test_prepare_select(fd_set main_set, int maxSocket, int parentfd){
	if (!test_flag)
		return;
	showSep();
	printf("_test_prepare_select:\n");
	showD("parentfd",parentfd);
	showD("maxSocket",maxSocket);
	showD("FD_ISSET(parentfd, &main_set)",FD_ISSET(parentfd, &main_set));

}
void _test_target_server_address_build(struct sockaddr_in targetaddr){
	if (!test_flag)
		return;
	showSep();
	printf("_test_target_server_address_build:\n");
	
	char *hostaddrp = inet_ntoa(targetaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    showS("targetaddr.sin_addr", hostaddrp);
	showD("targetaddr.sin_port", ntohs(targetaddr.sin_port));
}
void _test_address_is_same(struct sockaddr_in s1, struct sockaddr_in s2){
	if (!test_flag)
		return;
	showSep();
	printf("_test_address_is_same:\n");
	showD("same_addresses", address_is_same(s1, s1));
	showD("same_addresses", address_is_same(s2, s2));
	showD("diff_addresses", address_is_same(s1, s2));
	
	struct sockaddr_in s3;
	struct sockaddr_in s4;
	target_server_address_build("localhost", 9999, &s3);
	target_server_address_build("localhost", 9999, &s4);
	showD("diff_addresses", address_is_same(s1, s3));
	showD("same_addresses", address_is_same(s4, s3));
}
void _test_client_is_new(struct sockaddr_in s1, struct sockaddr_in s2){
	if (!test_flag)
		return;
	showSep();
	printf("_test_client_is_new:\n");
	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0;	
	client_list[client_list_counter++] = s1;
	showD("client_list_counter(1)", client_list_counter);
	int new = client_is_new(s2, client_list, client_list_counter);
	showD("is new", new);
	int old = client_is_new(s1, client_list, client_list_counter);
	showD("is NOT new", old);	
	client_list[client_list_counter++] = s2;
	showD("client_list_counter(2)", client_list_counter);
	int oldd = client_is_new(s2, client_list, client_list_counter);
	showD("is NOT new", oldd);	
	free(client_list);	
}
void _test_store_one_address(struct sockaddr_in s1, struct sockaddr_in s2){
	if (!test_flag)
		return;
	showSep();
	printf("_test_store_one_address:\n");
	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0;	
	int testNo = 1;
	
	int r1 = store_one_address(s1, client_list, &client_list_counter);
	
	if (client_list_counter==1 && r1==1){
		showPass(testNo++);
	} else {showFail(testNo++);}
	
	int r2 = store_one_address(s1, client_list, &client_list_counter);
	if (client_list_counter==1 && r2==0){
		showPass(testNo++);
	} else {showFail(testNo++);}
	
	int r3 = store_one_address(s2, client_list, &client_list_counter);
	if (client_list_counter==2 && r3==1){
		showPass(testNo++);
	} else {showFail(testNo++);}
	
	int r4 = store_one_address(s2, client_list, &client_list_counter);
	if (client_list_counter==2 && r4==0){
		showPass(testNo++);
	} else {showFail(testNo++);}
	
	free(client_list);
	client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	client_list_counter = 0;
	int r5 = store_one_address(s1, client_list, &client_list_counter);
	if (client_list_counter==1 && r5==1){
		showPass(testNo++);
	} else {showFail(testNo++);}
	
	int r6 = store_one_address(s2, client_list, &client_list_counter);
	if (client_list_counter==2 && r6==1){
		showPass(testNo++);
	} else {showFail(testNo++);}
	
	struct sockaddr_in s3;
	target_server_address_build("localhost", 9999, &s3);
	/*
	// #define ARRAYSIZE 2 
	int r7 = store_one_address(s3, client_list, &client_list_counter);
	if (client_list_counter==2 && r7==-1){
		showPass(testNo++);
	} else {showFail(testNo++);}
	*/
	free(client_list);	
}

void _test_construct_inital_hello(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_inital_hello:\n");
	
	char buf[BUFSIZE];
	
	int len = construct_inital_hello(buf, 0);
	int testNo = 1;
	if (len != 2 || buf[0] != 1 || buf[1] != 0){
		showFail(testNo);
		return;
	}
	
	len = construct_inital_hello(buf, 1);
	testNo = 1;
	if (len != 2 || buf[0] != 1 || buf[1] != 1){
		showFail(testNo);
		return;
	}
	/*	
	
	struct sockaddr_in s1;
	target_server_address_build("localhost", 9999, &s1);
	
	int len = construct_inital_hello(buf, s1, 0);
	int testNo = 1;
	if (len != 18 || buf[0] != 1 || buf[17] != 0){
		showFail(testNo);
		return;
	}
	testNo = 2;
	struct sockaddr_in* ptr = (struct sockaddr_in *)(&(buf[1]));
	if (address_is_same(*ptr, s1) != 1){
		showFail(testNo);
	}
	
	len = construct_inital_hello(buf, s1, 1);
	testNo = 3;
	if (len != 18 || buf[0] != 1 || buf[17] != 1){
		showFail(testNo);
		return;
	}
	testNo = 4;
	ptr = (struct sockaddr_in *)(&(buf[1]));
	if (address_is_same(*ptr, s1) != 1){
		showFail(testNo);
	}
	*/
}
void _test_state_signal_is_same(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_state_signal_is_same:\n");
	
	int play_state = 0;
	int play_signal = 0;
	int same = state_signal_is_same(play_state, play_signal);
	
	int testNo=1;
	if (same != 1){
		showFail(testNo);
		return;
	}
	testNo=2;
	play_state = 1;
	play_signal = 0;
	same = state_signal_is_same(play_state, play_signal);
	if (same != 0){
		showFail(testNo);
		return;
	}
	testNo=2;
	play_state = 0;
	play_signal = 1;
	same = state_signal_is_same(play_state, play_signal);
	if (same != 0){
		showFail(testNo);
		return;
	}
}
void _test_construct_pause(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_pause:\n");
	
	char buf[BUFSIZE];
	struct timeval time_stamp;
	time_stamp.tv_sec = 3;
	time_stamp.tv_usec = 500000;
	int len;
	int testNo = 1;
	struct timeval *ptr;
	
	testNo = 1;
	len = construct_pause(buf, time_stamp);
	if (len != 17 || buf[0]!=3){
		showFail(testNo);
		return;
	}
	
	testNo = 2;
	ptr = (struct timeval *)(&(buf[1]));
	if (timeS_is_same(*ptr, time_stamp) != 1){
		showFail(testNo);
		return;
	}
	
	testNo = 3;
	time_stamp.tv_sec = 3;
	time_stamp.tv_usec = 500001;
	if (timeS_is_same(*ptr, time_stamp) != 0){
		showFail(testNo);
		return;
	}
}
void _test_construct_play(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_play:\n");
	
	char buf[BUFSIZE];
	struct timeval time_stamp;
	time_stamp.tv_sec = 100;
	time_stamp.tv_usec = 6000;
	int len;
	int testNo = 1;
	struct timeval *ptr;
	
	testNo = 1;
	len = construct_play(buf, time_stamp);
	if (len != 17 || buf[0]!=4){
		showFail(testNo);
		return;
	}
	
	testNo = 2;
	ptr = (struct timeval *)(&(buf[1]));
	if (timeS_is_same(*ptr, time_stamp) != 1){
		showFail(testNo);
		return;
	}
	
	testNo = 3;
	time_stamp.tv_sec = 3;
	time_stamp.tv_usec = 500001;
	if (timeS_is_same(*ptr, time_stamp) != 0){
		showFail(testNo);
		return;
	}
}
void _test_send_to_all(int sockfd, struct sockaddr_in *client_list, int client_list_counter){
	if (!test_flag)
		return;
	showSep();
	printf("_test_send_to_all:\n");
	showD("client_list_counter", client_list_counter);
	
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int buf_len = 0;
	int testNo;
	buf[buf_len++] = 'a';
	buf[buf_len++] = 'b';
	buf[buf_len++] = 'c';
	int repeat_time = 2;
	int result = send_to_all(sockfd, buf, buf_len, client_list, client_list_counter, repeat_time);	
	testNo = 1;
	showD("result", result);
}
void _test_construct_userlist(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_userlist:\n");
	int r;
	int len;
	int testNo;
	struct sockaddr_in *ptr;
	int index;
	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0;
	
	struct sockaddr_in s1;
	target_server_address_build("localhost", 9999, &s1);
	r = store_one_address(s1, client_list, &client_list_counter);
	
	struct sockaddr_in s2;
	target_server_address_build("localhost", 2123, &s2);
	r = store_one_address(s2, client_list, &client_list_counter);
	
	struct sockaddr_in s3;
	target_server_address_build("localhost", 1000, &s3);
	r = store_one_address(s3, client_list, &client_list_counter);
	
	/* start */
	char buf[BUFSIZE];
	len = construct_userlist(buf, client_list, client_list_counter);
	
	testNo = 1;
	if (len != (1+3*sizeof(struct sockaddr_in)) || buf[0]!=2){
		showFail(testNo);
		return;
	}
	
	testNo = 2;
	ptr = (struct sockaddr_in *)(&(buf[1]));
	if (address_is_same(*ptr, s1) != 1){
		showFail(testNo);
		return;
	}
	
	testNo = 3;
	index = 1 + sizeof(struct sockaddr_in);
	ptr = (struct sockaddr_in *)(&(buf[index]));
	if (address_is_same(*ptr, s2) != 1){
		showFail(testNo);
		return;
	}
	
	testNo = 4;
	index = 1 + sizeof(struct sockaddr_in) * 2;
	ptr = (struct sockaddr_in *)(&(buf[index]));
	if (address_is_same(*ptr, s3) != 1){
		showFail(testNo);
		return;
	}
	
	
	free(client_list);
}









