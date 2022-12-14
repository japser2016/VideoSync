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
#include <time.h>

#include "p_client_io.h"

#define BUFSIZE 1024
#define ARRAYSIZE 32
// each struct sockaddr_in has size 16 bytes -> 32*16 = 512 bytes
#define test_flag 1
#define REPEAT 1
#define TIMEOUT 0  //for select timeout in seconds
#define TIMEOUT_USEC 10  //for select timeout in usec (1/1000000 second)


/*
 * error - wrapper for perror
 */
/* void error(char *msg) { */
/*   perror(msg); */
/*   exit(1); */
/* } */

/*
 * data structures
 */
struct WaitNode{
	int time_out_count;
	time_t since; //time(NULL) -> get current time 
	struct sockaddr_in client;	
};

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
int store_address_list(struct sockaddr_in *out_client_address_list, int out_client_address_list_counter, struct sockaddr_in *client_list, int *client_list_counter);
void main_loop(fd_set *main_set, int *maxSocket, int parentfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in *client_list, int *client_list_counter, int *need_file, struct sockaddr_in serveraddr, char *vidLoc);
int state_signal_is_same(int play_state, int play_signal);
void store_timestamp(struct timeval *time_stamp, struct timeval target_stamp);
int check_msg_type(char *buf, int msg_len);
void request_videofile(struct sockaddr_in serveraddr, struct sockaddr_in clientaddr,
                       struct sockaddr_in *client_list, int client_list_counter, int parentfd);
int client_need_file(char *buf);
void first_initial_hello_pkt_lost(int parentfd, int timeout, struct sockaddr_in *client_list, int *client_list_counter, int *need_file, struct sockaddr_in serveraddr, struct sockaddr_in targetaddr);
void construct_file_buf(char *file_buf, int play_signal, struct timeval time_stamp, int seq_no);

void check_file(int *play_state, int *play_signal, struct timeval *time_stamp, char *player_file_name, int *seq_no);

/* functions handle msg received */
void handle_initial_hello(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct sockaddr_in clientaddr);
void handle_userlist(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct sockaddr_in clientaddr, char *msg, int msg_len);
void handle_pause(int sockfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in clientaddr, char *msg, int msg_len);
void handle_play(int sockfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in clientaddr, char *msg, int msg_len);

int add_one_WL_node(struct WaitNode *wait_list, int *WL_counter, struct sockaddr_in clientaddr);
int remove_one_node_from_WL(struct WaitNode *wait_list, int *WL_counter, struct sockaddr_in clientaddr);
int check_resend_user_list(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct WaitNode *wait_list, int WL_counter, int resend_time_sec, int resend_times);
int check_resend_pause_play(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct WaitNode *wait_list, int WL_counter, int resend_time_sec, int resend_times, struct timeval time_stamp, int signal_type);
void update_file(int play_state, int play_signal, struct timeval time_stamp, int *seq_no, char *my_file_name);


/* send functions */
int sends(int sockfd, char *buf, int buf_len, struct sockaddr_in targetaddr);
int send_to_all(int sockfd, char *buf, int buf_len, struct sockaddr_in *client_list, int client_list_counter, int repeat_time);
int construct_initial_hello(char *buf, int need_file);
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
void showTimestamp(struct timeval tv);
int address_is_same(struct sockaddr_in s1, struct sockaddr_in s2);
int client_is_new(struct sockaddr_in client_address, struct sockaddr_in *client_list, int client_list_counter);
int timeS_is_same(struct timeval t1, struct timeval t2);
int copy_clients_without(struct sockaddr_in *client_list_c, int *client_list_counter_c, struct sockaddr_in *client_list, int client_list_counter, struct sockaddr_in except);
void showT(int type);
int wait_node_is_new(struct WaitNode node, struct WaitNode *wait_list, int WL_counter);
double timestamp_to_double(struct timeval tv);
void double_to_timestamp(struct timeval *tv, double db);


 
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
void _test_construct_initial_hello();
//void _test_sends();
void _test_state_signal_is_same();
void _test_construct_pause();
void _test_construct_play();
void _test_send_to_all(int sockfd, struct sockaddr_in *client_list, int client_list_counter);
void _test_construct_userlist();
void _test_construct_userlist_ack();
void _test_construct_pause_ack();
void _test_construct_play_ack();
void _test_copy_clients_without();
void _test_store_address_list();
void _test_store_timestamp();
void _test_set_timeout_address();
void _test_add_remove_WL();

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

	if (!need_file)
		system("cd video_player; node index.js &");
	
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
		
		_test_construct_initial_hello();
		len = construct_initial_hello(buf, need_file);
		/* send INITIAL HELLO - REPEAT times */
        sends(parentfd, buf, len, targetaddr);
        bzero(buf,BUFSIZE);
        len = construct_initial_hello(buf, 0);
		for (int i = 0; i < REPEAT-1; i++){
			sends(parentfd, buf, len, targetaddr);
		}
		////
		int time_out_sec = 3; 
		first_initial_hello_pkt_lost(parentfd, time_out_sec, client_list, &client_list_counter, &need_file, serveraddr, targetaddr);
	}
	main_loop(&main_set, &maxSocket, parentfd, &play_state, &play_signal, &time_stamp, client_list, &client_list_counter, &need_file, serveraddr, vidLoc);
	
	
	free(client_list);
	client_list_counter = 0;
	system("killall node");
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
 * return 0 if complete, else -1
 */
int set_timeout_address(int parentfd, int enable, int sec){
	int result = -1;
	struct timeval tv;
	if (enable == 0){
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	} else {
		tv.tv_sec = sec;
		tv.tv_usec = 0;
	}
	result = setsockopt(parentfd, SOL_SOCKET, SO_RCVTIMEO, 
						(char *)&tv , sizeof(tv));	
	return result;
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
int store_address_list(struct sockaddr_in *out_client_address_list, int out_client_address_list_counter, struct sockaddr_in *client_list, int *client_list_counter){
	for (int i = 0; i < out_client_address_list_counter; i++){
		client_list[(*client_list_counter)++] = out_client_address_list[i];
	}
	return *client_list_counter;
}




/*
 * the main loop to handle 
 * user's commands (play/pause)
 * msg received from other peers 
 * eg. initial hello, user list, pause/play signal
 */
void main_loop(fd_set *main_set, int *maxSocket, int parentfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in *client_list, int *client_list_counter, int *need_file, struct sockaddr_in serveraddr, char *vidLoc){
	/* tests */
	_test_state_signal_is_same();
	_test_construct_pause();
	_test_construct_play();
	/* this _test_send_to_all would just send to parent, use the one in the while loop to test*/
	//_test_send_to_all(parentfd, client_list, *client_list_counter);
	_test_construct_userlist();
	_test_construct_userlist_ack();
	_test_construct_pause_ack();
	_test_construct_play_ack();
	_test_copy_clients_without();
	_test_store_address_list();
	_test_store_timestamp();
	//_test_set_timeout_address();
	_test_add_remove_WL();
	
	
	/*******************************************/
    /**************** error handle *************/
	int resend_time_sec = 3; /* wait time (in seconds) before resending */
	int resend_times = 3; /* resend times */
	/* wait list for clients that should send USER LIST ACK */
	struct WaitNode *WL_user_list_ack = malloc(ARRAYSIZE * sizeof(struct WaitNode)); 
	bzero(WL_user_list_ack, ARRAYSIZE * sizeof(struct WaitNode));
	int WL_user_list_ack_counter = 0;
	/* wait list for clients that should send PAUSE ACK */
	struct WaitNode *WL_pause_ack = malloc(ARRAYSIZE * sizeof(struct WaitNode)); 
	bzero(WL_pause_ack, ARRAYSIZE * sizeof(struct WaitNode));
	int WL_pause_ack_counter = 0;
	/* wait list for clients that should send PLAY ACK */
	struct WaitNode *WL_play_ack = malloc(ARRAYSIZE * sizeof(struct WaitNode)); 
	bzero(WL_play_ack, ARRAYSIZE * sizeof(struct WaitNode));
	int WL_play_ack_counter = 0;
	
	/*************** files **********************/
	int seq_no = 0;
	char my_file_name[100] = "syncserver-output";
	char player_file_name[100] = "webserver-output";
	FILE *my_file = fopen(my_file_name, "w+");
    FILE *player_file = fopen(player_file_name, "w+");
	/* file string buf */
	char file_buf[BUFSIZE];
	bzero(file_buf, BUFSIZE);	
	construct_file_buf(file_buf, *play_signal, *time_stamp, seq_no);	
	fwrite(file_buf, 1, strlen(file_buf), my_file);
    fwrite(file_buf, 1, strlen(file_buf), player_file);
	if (my_file != NULL)
		fclose(my_file);
	if (player_file != NULL)
        fclose(player_file);
	/* main while */
	fd_set temp_set;
	int readyNo = 0;	
	int result;
	while(1){
		//_test_send_to_all(parentfd, client_list, *client_list_counter);
		
		/***************************************************/
        
		check_file(play_state, play_signal, time_stamp, player_file_name, &seq_no);
		
		int same = state_signal_is_same(*play_state, *play_signal);		
		if (same == 0){
			if (*play_signal == 0){
				/* set play state */
				*play_state = 0;
				/* store/change timestamp */
				/*********************************/
				/*
				struct timeval FAKE_STAMP; // need to be replaced by real time stamp from player
				bzero(&FAKE_STAMP,sizeof(struct timeval));
				FAKE_STAMP.tv_sec = 100;
				FAKE_STAMP.tv_usec = 500000;
				/*********************************/				
				//store_timestamp(time_stamp, FAKE_STAMP);
				
				/* construct and send PAUSE */
				char buf[BUFSIZE];
				bzero(buf, BUFSIZE);
				int buf_len = construct_pause(buf, *time_stamp);
				send_to_all(parentfd, buf, buf_len, client_list, *client_list_counter, REPEAT);
				
				/* reset wait list */
				bzero(WL_pause_ack, ARRAYSIZE * sizeof(struct WaitNode));
				WL_pause_ack_counter = 0;
				/* add all clients into the pause ack wait list */
				for (int i = 0; i < *client_list_counter; i++ ){
					add_one_WL_node(WL_pause_ack, &WL_pause_ack_counter, client_list[i]);
				}
				
			} else {
				/* store/change timestamp */
				/*********************************/
				/*
				struct timeval FAKE_STAMP; // need to be replaced by real time stamp from player
				bzero(&FAKE_STAMP,sizeof(struct timeval));
				FAKE_STAMP.tv_sec = 100;
				FAKE_STAMP.tv_usec = 500000;
				/*********************************/					
				//store_timestamp(time_stamp, FAKE_STAMP);
				
				/* construct and send PLAY */
				char buf[BUFSIZE];
				bzero(buf, BUFSIZE);
				int buf_len = construct_play(buf, *time_stamp);
				send_to_all(parentfd, buf, buf_len, client_list, *client_list_counter, REPEAT);
				/* set play state */
				*play_state = 1;
				
				/* reset wait list */
				bzero(WL_play_ack, ARRAYSIZE * sizeof(struct WaitNode));
				WL_play_ack_counter = 0;
				/* add all clients into the play ack wait list */
				for (int i = 0; i < *client_list_counter; i++ ){
					add_one_WL_node(WL_play_ack, &WL_play_ack_counter, client_list[i]);
				}
			}
		}
		
		temp_set = *main_set;
		/* set select() time out interval to TIMEOUT seconds */
		struct timeval timeout;
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = TIMEOUT_USEC;
		//printf("before select\n");
		readyNo = select(*maxSocket+1, &temp_set, NULL, NULL, &timeout);
		//printf("after select\n");
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
			if (test_flag == 1){
				showSep();
				showD("len(received)", len);
				showS("buf", buf);
				showD("msgType", msg_type);	
				showT(msg_type);
				showD("client_list_counter", *client_list_counter);
			}			
			/* handle received messages */
			switch(msg_type){
				case 1:
					/* if INITIAL HELLO */
					handle_initial_hello(parentfd, client_list, client_list_counter, clientaddr);
                    if (*need_file == 0 && client_need_file(buf))
                        serve_videofile(serveraddr, clientaddr, vidLoc);
					
					/* add client to the wait list */
					result = add_one_WL_node(WL_user_list_ack, &WL_user_list_ack_counter, clientaddr);				
					break;
				case 2:
					/* if USER LIST */
					handle_userlist(parentfd, client_list, client_list_counter, clientaddr, buf, len);
                    if (*need_file) {
                        request_videofile(serveraddr, clientaddr, client_list, *client_list_counter, parentfd);
                        *need_file = 0;
						system("cd video_player; node index.js &");
                    }
					break;
				case 3:
					/* if PAUSE */
					handle_pause(parentfd, play_state, play_signal, time_stamp, clientaddr, buf, len);
					update_file(*play_state, *play_signal, *time_stamp, &seq_no, my_file_name);
					break;
				case 4:
					/* if PLAY */
					handle_play(parentfd, play_state, play_signal, time_stamp, clientaddr, buf, len);
					update_file(*play_state, *play_signal, *time_stamp, &seq_no, my_file_name);
					break;
				case 5:
					/* if USER LIST ACK */					
					result = remove_one_node_from_WL(WL_user_list_ack, &WL_user_list_ack_counter, clientaddr);
				case 6:
					/* if PAUSE ACK */					
					result = remove_one_node_from_WL(WL_pause_ack, &WL_pause_ack_counter, clientaddr);
				case 7:
					/* if PLAY ACK */					
					result = remove_one_node_from_WL(WL_play_ack, &WL_play_ack_counter, clientaddr);
					
			}

            
		}
		/* check ACKs and resend data accordingly */
		/* USER LIST*/
		check_resend_user_list(parentfd, client_list, client_list_counter, WL_user_list_ack, WL_user_list_ack_counter, resend_time_sec, resend_times);
		/* PAUSE */
		check_resend_pause_play(parentfd, client_list, client_list_counter, WL_pause_ack, WL_pause_ack_counter, resend_time_sec, resend_times, *time_stamp, 0);
		/* PLAY */
		check_resend_pause_play(parentfd, client_list, client_list_counter, WL_play_ack, WL_play_ack_counter, resend_time_sec, resend_times, *time_stamp, 1);
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
void store_timestamp(struct timeval *time_stamp, struct timeval target_stamp){
	bzero(time_stamp, sizeof(struct timeval));
	(*time_stamp).tv_sec = target_stamp.tv_sec;
	(*time_stamp).tv_usec = target_stamp.tv_usec;
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

void request_videofile(struct sockaddr_in serveraddr, struct sockaddr_in clientaddr,
                       struct sockaddr_in *client_list, int client_list_counter, int parentfd) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    if (connect(sockfd, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
        int i = 0;
        for (i = 0; i < client_list_counter; i++) {
            char buf[BUFSIZE];
            int len = construct_initial_hello(buf, 1);
            sends(parentfd, buf, len, client_list[i]);
            if (connect(sockfd, (const struct sockaddr *)&(client_list[i]), sizeof(client_list[i])) >= 0) {
                receive_video(sockfd);
                break;
            }
        }
    } else
        receive_video(sockfd);
    close(sockfd);
}

int client_need_file(char *buf) {
    return (int)buf[1];
}

/* 
 * handle the cases where initial hello ack
 * is not received correctly
 */
void first_initial_hello_pkt_lost(int parentfd, int timeout, struct sockaddr_in *client_list, int *client_list_counter, int *need_file, struct sockaddr_in serveraddr, struct sockaddr_in targetaddr){
	int time_out_times = 0;
	
	while (1){
		/* receive the message */
		char buf[BUFSIZE];
		bzero(buf, BUFSIZE);
		struct sockaddr_in clientaddr;
		int clientlen = sizeof(clientaddr);
		bzero((char *) &clientaddr, sizeof(clientaddr));
		
		/* set time out */
		int result = -1;
		result = set_timeout_address(parentfd, 1, timeout);
		int len = recvfrom(parentfd, buf, BUFSIZE, 0,
				(struct sockaddr *) &clientaddr, &clientlen);
		int msg_type = check_msg_type(buf, len);
		/* show msg for debugging */
		if (test_flag == 1){
			showSep();
			printf("From first_initial_hello_pkt_lost\n");
			showD("result of set timeout", result);
			showD("len(received)", len);
			showS("buf", buf);
			showD("msgType", msg_type);	
			showT(msg_type);
			//showD("client_list_counter", *client_list_counter);
		}	
		/* cancel time out */
		result = set_timeout_address(parentfd, 0, 0);
		if (msg_type == 2){
			/* if USER LIST */
			handle_userlist(parentfd, client_list, client_list_counter, clientaddr, buf, len);
			if (*need_file) {
				request_videofile(serveraddr, clientaddr, client_list, *client_list_counter, parentfd);
				*need_file = 0;
				system("cd video_player; node index.js &");
			}
			break;
		} else {
			printf("--------------TIME OUT---------------\n");
			time_out_times += 1;
			/* resend the initial hello */
			bzero(buf, BUFSIZE);
			len = construct_initial_hello(buf, *need_file);
			sends(parentfd, buf, len, targetaddr);
			for (int i = 0; i < REPEAT-1; i++){
				sends(parentfd, buf, len, targetaddr);
			}
		}
		
		if (time_out_times >= 3){
			error("ERROR connecting to main client");
		}
	}
}

/* 
 * construct 3 line file buf according to the inputs
 */
void construct_file_buf(char *file_buf, int play_signal, struct timeval time_stamp, int seq_no){
	bzero(file_buf, BUFSIZE);
	/* first line */
	sprintf(file_buf, "%d", play_signal);
	strcat(file_buf, "\n");
	/* second line */
	double timestamp_double = timestamp_to_double(time_stamp);
	char temp_string[BUFSIZE];
	bzero(temp_string, BUFSIZE);
	snprintf(temp_string, BUFSIZE, "%f\n", timestamp_double);
	strcat(file_buf, temp_string);
	/* third line */
	bzero(temp_string, BUFSIZE);
	sprintf(temp_string, "%d\n", seq_no);
	strcat(file_buf, temp_string);		
}

void check_file(int *play_state, int *play_signal, struct timeval *time_stamp, char *player_file_name, int *seq_no){
	/*
	showSep();
	printf("in check_file\n");
	*/
	FILE *other_side_file = fopen(player_file_name, "r");
	char *play_state_line = NULL;
	char *timestamp_line = NULL;
	char *seq_no_line = NULL;
	size_t len = 0;
	ssize_t read;
	int play_state_f = *play_state;
	double timestamp_double_f = timestamp_to_double(*time_stamp);
    showSep();
    //printf("current timestamp: %f\n",timestamp_double_f);
	int seq_no_f = *seq_no;
	if (other_side_file != NULL){
		if ((read = getline(&play_state_line, &len, other_side_file)) != -1){
			play_state_line[1] = '\0';
			sscanf(play_state_line, "%d", &play_state_f);
		}
		if ((read = getline(&timestamp_line, &len, other_side_file)) != -1){
            //printf("timestamp line: %s\n",timestamp_line);
			int last_index = strlen(timestamp_line)-2;
			//timestamp_line[last_index] = '\0';
            char *end;
			timestamp_double_f = strtod(timestamp_line,&end);
            //printf("changing timestamp to: %f\n",timestamp_double_f);
		}
		if ((read = getline(&seq_no_line, &len, other_side_file)) != -1){
			int last_index = strlen(seq_no_line)-1;
			seq_no_line[last_index] = '\0';
			sscanf(seq_no_line, "%d", &seq_no_f);
		}
		if (seq_no_f > *seq_no){
			/* update seq_no & play_signal & time_stamp */
			*seq_no = seq_no_f;
			*play_signal = play_state_f;
			double_to_timestamp(time_stamp, timestamp_double_f);
		}
		if (play_state_line)
			free(play_state_line);
		if (timestamp_line)
			free(timestamp_line);
		if (seq_no_line)
			free(seq_no_line);
		fclose(other_side_file);
	}
    /*
	showD("play_state", *play_state);
	showD("play_signal", *play_signal);
	showD("seq_no", *seq_no);
	*/
}


/* functions handle msg received */

/*
 * NOT send HELLO ACK 
 * send client list
 * store new client
 */
void handle_initial_hello(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct sockaddr_in clientaddr){
	int new = client_is_new(clientaddr, client_list, *client_list_counter);
	if ( new == 1){
		/* send client list */
		char buf[BUFSIZE];
		bzero(buf, BUFSIZE);
		int buf_len = construct_userlist(buf, client_list, *client_list_counter);
		int send_bytes = sends(sockfd, buf, buf_len, clientaddr);
		/* store new client */
		int store_result = store_one_address(clientaddr, client_list, client_list_counter);
	} else {
		/* copy a client list without current client */
		struct sockaddr_in *client_list_c = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
		bzero(client_list_c, ARRAYSIZE * sizeof(struct sockaddr_in));
		int client_list_counter_c = 0;
		int copied_num = copy_clients_without(client_list_c, &client_list_counter_c, client_list, *client_list_counter, clientaddr);
		/* send client list */
		char buf[BUFSIZE];
		bzero(buf, BUFSIZE);
		int buf_len = construct_userlist(buf, client_list_c, client_list_counter_c);
		int send_bytes = sends(sockfd, buf, buf_len, clientaddr);	
		
		free(client_list_c);		
	}
}

/*
 * send USER LIST ACK
 * store user list if local list is new (with only one peer)
 */
void handle_userlist(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct sockaddr_in clientaddr, char *msg, int msg_len){
	/* send USER LIST ACK */
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int buf_len;
	buf_len = construct_userlist_ack(buf);	
	int send_bytes = sends(sockfd, buf, buf_len, clientaddr);
	/* store the received user list if I am new (with only parent in the client list)*/
	if ((*client_list_counter) <= 1 && msg_len > 1){
		/* load outside client list from msg to client_list_c */
		struct sockaddr_in *client_list_c = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
		bzero(client_list_c, ARRAYSIZE * sizeof(struct sockaddr_in));
		int client_list_counter_c = 0;
		
		client_list_counter_c = (msg_len-1)/sizeof(struct sockaddr_in);
		int copy_size = msg_len-1;
		memcpy(client_list_c, &(msg[1]), copy_size);
		
		/* store the outside cliend list */
		int result_list_counter = store_address_list(client_list_c, client_list_counter_c, client_list, client_list_counter);
		
		/******************BASIC TEST**************************/
		/*
		showSep();
		printf("_test_inside_handle_userlist part 1:\n");
		showD("result_list_counter(2)", result_list_counter);
		showD("client_list_counter(2)", *client_list_counter);
		int same = address_is_same(client_list[0], clientaddr);
		showD("client_list[0] is same(1)", same);
		same = address_is_same(client_list[1], *((struct sockaddr_in*)(&(msg[1]))));
		showD("client_list[1] is same(1)", same);
		*/
		/************************************************/
		
		/* send INITIAL HELLO to all new friends EXCEPT the parent */
		bzero(buf, BUFSIZE);
		int need_file = 0; // change to 1 to ask for file from everyone
		buf_len = construct_initial_hello(buf, 0);
		send_to_all(sockfd, buf, buf_len, &(client_list[1]), *client_list_counter-1, REPEAT);
		
		/* free copied list */
		free(client_list_c);
	}
	
}

/*
 * send PAUSE ACK
 * set the time stamp (according to the message)
 * change play state and signal if diff
 */
void handle_pause(int sockfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in clientaddr, char *msg, int msg_len){
	/* send PAUSE ACK */
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int buf_len;
	buf_len = construct_pause_ack(buf);
	int send_bytes = sends(sockfd, buf, buf_len, clientaddr);
	/* set time stamp according to the msg*/
	if (msg_len != 17){
		printf("\nhandle_pause: PAUSE msg's size is wrong! is: %d; should be 17\n", msg_len);
		return;
	}
	store_timestamp(time_stamp, *((struct timeval *)(&(msg[1]))));
	/* change play state to 0 (pause) if currently not */
	if (*play_state == 1){
		*play_signal = 0;
		*play_state = 0;
	}
	/**************** test *****************/
	if (test_flag == 1){
		showSep();
		printf("_test_inside_handle_pause:\n");
		showD("play_signal", *play_signal);
		showD("play_state", *play_state);
		showTimestamp(*time_stamp);
	}
}

/*
 * send PLAY ACK
 * set the time stamp (according to the message)
 * change play state and signal if diff
 */
void handle_play(int sockfd, int *play_state, int *play_signal, struct timeval *time_stamp, struct sockaddr_in clientaddr, char *msg, int msg_len){
	/* send PLAY ACK */
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int buf_len;
	buf_len = construct_play_ack(buf);
	int send_bytes = sends(sockfd, buf, buf_len, clientaddr);
	/* set time stamp according to the msg*/
	if (msg_len != 17){
		printf("\nhandle_play: PLAY msg's size is wrong! is: %d; should be 17\n", msg_len);
		return;
	}
	store_timestamp(time_stamp, *((struct timeval *)(&(msg[1]))));
	/* change play state to 1 (pause) if currently not */
	if (*play_state == 0){
		*play_signal = 1;
		*play_state = 1;
	}
	/**************** test *****************/
	if (test_flag == 1){
		showSep();
		printf("_test_inside_handle_play:\n");
		showD("play_signal", *play_signal);
		showD("play_state", *play_state);
		showTimestamp(*time_stamp);
	}
	
}
/*
 * initialize current client (clientaddr) WaitNode
 * check if exsted
 * if new, store the client (WaitNode) into the wait list
 * return 1 if success, 0 if not success, -1 if ARRAYSIZE not enough
 */
int add_one_WL_node(struct WaitNode *wait_list, int *WL_counter, struct sockaddr_in clientaddr){
	/* initialize current client WaitNode */
	struct WaitNode node;
	bzero((char *) &node, sizeof(struct WaitNode));
	node.time_out_count = 0;
	node.since = time(NULL);
	node.client = clientaddr;
	/* store the WaitNode to the list */
	if (wait_node_is_new(node, wait_list, *WL_counter) == 1){
		if (*WL_counter < ARRAYSIZE){
			wait_list[(*WL_counter)++] = node;
			return 1;
		}
		printf("add_one_WL_node: add wait list failed -> ARRAYSIZE %d is not enough\n", ARRAYSIZE);
		return -1;
	}
	printf("add_one_WL_node: WaitNode existed\n", ARRAYSIZE);
	return 0;
}

/* 
 * return 1 if success, return 0 if nothing removed
 */
int remove_one_node_from_WL(struct WaitNode *wait_list, int *WL_counter, struct sockaddr_in clientaddr){
	struct WaitNode *WL_copy = malloc(ARRAYSIZE * sizeof(struct WaitNode)); 
	bzero(WL_copy, ARRAYSIZE * sizeof(struct WaitNode));
	int WL_copy_counter = 0;
	
	for (int i = 0; i < *WL_counter; i++){
		if (address_is_same(clientaddr, wait_list[i].client) != 1){
			/* not the target */
			WL_copy[WL_copy_counter++] = wait_list[i];
		}
	}
	if (WL_copy_counter == (*WL_counter)){
		/* nothing removed */
		return 0;
	}
	
	/* copy back to the original wait_list */
	bzero(wait_list, ARRAYSIZE * sizeof(struct WaitNode));
	(*WL_counter) = 0;
	for (int i = 0; i < WL_copy_counter; i++){
		wait_list[(*WL_counter)++] = WL_copy[i];
	}
	
	free(WL_copy);
	return 1;
}

/*
 * check if ACK timeout resend USER LIST
 * return resend number
 */
int check_resend_user_list(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct WaitNode *wait_list, int WL_counter, int resend_time_sec, int resend_times){
	int resend_number = 0;
	for (int i = 0; i < WL_counter; i++){
		time_t current_time = time(NULL);
		if ((current_time - wait_list[i].since) >= resend_time_sec && wait_list[i].time_out_count < resend_times ){
			printf("resend USER LIST\n");
			/* resend USER LIST */
			handle_initial_hello(sockfd, client_list, client_list_counter, wait_list[i].client);
			/* update WaitNode */
			(wait_list[i].time_out_count)++;
			wait_list[i].since = time(NULL);
			/* increase resend number */
			resend_number++;
		}
	}
	return resend_number;
}

/*
 * check if ACK timeout 
 * resend PAUSE(signal_type=0)/PLAY (signal_type=1)
 * return resend number
 */
int check_resend_pause_play(int sockfd, struct sockaddr_in *client_list, int *client_list_counter, struct WaitNode *wait_list, int WL_counter, int resend_time_sec, int resend_times, struct timeval time_stamp, int signal_type){
	int resend_number = 0;
	for (int i = 0; i < WL_counter; i++){
		time_t current_time = time(NULL);
		char buf[BUFSIZE];
		bzero(buf, BUFSIZE);
		int buf_len;
		if ((current_time - wait_list[i].since) >= resend_time_sec && wait_list[i].time_out_count < resend_times ){
			if (signal_type == 0){
				printf("resend PAUSE\n");				
				/* construct PAUSE */
				buf_len = construct_pause(buf, time_stamp);
			} else {
				printf("resend PLAY\n");
				/* construct PLAY */
				buf_len = construct_play(buf, time_stamp);
			}
			/* resend signal */
			sends(sockfd, buf, buf_len, wait_list[i].client);
			
			/* update WaitNode */
			(wait_list[i].time_out_count)++;
			wait_list[i].since = time(NULL);
			/* increase resend number */
			resend_number++;
		}
	}
	return resend_number;
}

void update_file(int play_state, int play_signal, struct timeval time_stamp, int *seq_no, char *my_file_name){
	FILE *my_file = fopen(my_file_name, "w+");
	char file_buf[BUFSIZE];
	bzero(file_buf, BUFSIZE);	
	(*seq_no)++;
	construct_file_buf(file_buf, play_signal, time_stamp, *seq_no);
	fwrite(file_buf, 1, strlen(file_buf), my_file);
	if (my_file != NULL)
		fclose(my_file);
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
/* 
 * send the buf to all the clients in the client list (with repeat)
 * return number of clients sent (have not tested yet)
 */
int send_to_all(int sockfd, char *buf, int buf_len, struct sockaddr_in *client_list, int client_list_counter, int repeat_time){
	//check if store add the counter
	int i = 0;
	for (i = 0; i < client_list_counter; i++){
		for (int j = 0; j < repeat_time; j++){
			int len = sends(sockfd, buf, buf_len, client_list[i]);
		}
	}
	return i;
}
int construct_initial_hello(char *buf, int need_file){
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
	bzero(buf, BUFSIZE);
	int buf_len = 0;
	/* add msg type */
	buf[buf_len++] = (char)6;
	return buf_len;
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
	bzero(buf, BUFSIZE);
	int buf_len = 0;
	/* add msg type */
	buf[buf_len++] = (char)7;
	return buf_len;
}
/* when client_list empy, its buf_len would just be 1 */
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
	bzero(buf, BUFSIZE);
	int buf_len = 0;
	/* add msg type */
	buf[buf_len++] = (char)5;
	return buf_len;
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
void showTimestamp(struct timeval tv){
	printf("tv.sec->%li\n", tv.tv_sec);
	printf("tv.usec->%li\n", tv.tv_usec);
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
	/*********************************/
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
/* 
 * copy all other clients without the specifc one provided 
 * return how many clients copied
 */
int copy_clients_without(struct sockaddr_in *client_list_c, int *client_list_counter_c, struct sockaddr_in *client_list, int client_list_counter, struct sockaddr_in except){
	*client_list_counter_c = 0;
	bzero(client_list_c, ARRAYSIZE * sizeof(struct sockaddr_in));
	for (int i = 0; i < client_list_counter; i++){
		int same = address_is_same(client_list[i], except);
		if (same != 1){
			client_list_c[(*client_list_counter_c)++] = client_list[i];
		}
	}
	return *client_list_counter_c;
}

/* print the type of the msg received */
void showT(int type){
	switch(type){
		case 1:
			printf("   INITIAL HELLO\n");
			break;
		case 2:
			printf("   USER LIST\n");
			break;
		case 3:
			printf("   PAUSE\n");
			break;
		case 4:
			printf("   PLAY\n");
			break;
		case 5:
			printf("   UserList ACK\n");
			break;
		case 6:
			printf("   PAUSE ACK\n");
			break;
		case 7:
			printf("   PLAY ACK\n");
			break;
		case 8:
			printf("   VIDPKT\n");
			break;
		case 9:
			printf("   VIDPKT ACK\n");
			break;
	}
}

/* 1 for new, 0 for existed */
int wait_node_is_new(struct WaitNode node, struct WaitNode *wait_list, int WL_counter){
	for (int i = 0; i < WL_counter; i++){
		if (address_is_same(node.client, wait_list[i].client) == 1){
			return 0;
		}
	}
	return 1;
}

/* convert struct timeval to double */
double timestamp_to_double(struct timeval tv){	
	double result;
	result = (double)tv.tv_sec;
	result += ((double)tv.tv_usec)/1000000;
	return result;
}

/* convert double to struct timeval */
void double_to_timestamp(struct timeval *tv, double db){
	tv->tv_sec = (time_t)db;
	tv->tv_usec = (long int)((db-((double)(tv->tv_sec)))*1000000);
	return;
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

void _test_construct_initial_hello(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_initial_hello:\n");
	
	char buf[BUFSIZE];
	
	int len = construct_initial_hello(buf, 0);
	int testNo = 1;
	if (len != 2 || buf[0] != 1 || buf[1] != 0){
		showFail(testNo);
		return;
	}
	
	len = construct_initial_hello(buf, 1);
	testNo = 1;
	if (len != 2 || buf[0] != 1 || buf[1] != 1){
		showFail(testNo);
		return;
	}
	/*	
	
	struct sockaddr_in s1;
	target_server_address_build("localhost", 9999, &s1);
	
	int len = construct_initial_hello(buf, s1, 0);
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
	
	len = construct_initial_hello(buf, s1, 1);
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
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
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
	
	testNo = 5;
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	client_list_counter = 0;
	len = construct_userlist(buf, client_list, client_list_counter);
	if (len != 1 || buf[0]!=2){
		showFail(testNo);
		return;
	}
	
	free(client_list);
}
void _test_construct_userlist_ack(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_userlist_ack:\n");
	
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int buf_len;
	int testNo;
	
	testNo = 1;
	buf_len = construct_userlist_ack(buf);
	if (buf_len != 1 || buf[0] != 5){
		showFail(testNo);
		return;
	}
}
void _test_construct_pause_ack(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_pause_ack:\n");
	
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int buf_len;
	int testNo;
	
	testNo = 1;
	buf_len = construct_pause_ack(buf);
	if (buf_len != 1 || buf[0] != 6){
		showFail(testNo);
		return;
	}
}
void _test_construct_play_ack(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_construct_play_ack:\n");
	
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int buf_len;
	int testNo;
	
	testNo = 1;
	buf_len = construct_play_ack(buf);
	if (buf_len != 1 || buf[0] != 7){
		showFail(testNo);
		return;
	}
}
void _test_copy_clients_without(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_copy_clients_without:\n");
	int testNo = 0;
	int result_counter;
	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0;	
	
	struct sockaddr_in *client_list_c = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(client_list_c, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter_c = 0;
	
	struct sockaddr_in s0;
	target_server_address_build("localhost", 8888, &s0);
	struct sockaddr_in s1;
	target_server_address_build("localhost", 9999, &s1);
	struct sockaddr_in s2;
	target_server_address_build("localhost", 1000, &s2);
	struct sockaddr_in s3;
	target_server_address_build("localhost", 1500, &s3);
	struct sockaddr_in s11;
	target_server_address_build("localhost", 2000, &s11);
	
	/* load client_list */
	client_list[client_list_counter++] = s0;
	client_list[client_list_counter++] = s1;
	client_list[client_list_counter++] = s2;
	client_list[client_list_counter++] = s3;
	
	testNo = 1;
	result_counter = copy_clients_without(client_list_c, &client_list_counter_c, client_list, client_list_counter, s11);
	if (result_counter != 4){
		showFail(testNo);
		free(client_list);
		free(client_list_c);
		return;
	}
	testNo = 2;
	for (int i = 0; i < client_list_counter_c; i++){
		int same = address_is_same(client_list_c[i], client_list[i]);
		if (same != 1){
			showFail(testNo);
			free(client_list);
			free(client_list_c);
			return;
		}
	}
	
	testNo = 3;
	result_counter = copy_clients_without(client_list_c, &client_list_counter_c, client_list, client_list_counter, s1);
	if (result_counter != 3){
		showFail(testNo);
		free(client_list);
		free(client_list_c);
		return;
	}
	testNo = 4;
	int same = address_is_same(client_list_c[0], client_list[0]);
	if (same != 1){
		showFail(testNo);
		free(client_list);
		free(client_list_c);
		return;
	}
	for (int i = 1; i < client_list_counter_c; i++){
		int same = address_is_same(client_list_c[i], client_list[i+1]);
		if (same != 1){
			showFail(testNo);
			free(client_list);
			free(client_list_c);
			return;
		}
	}
	
	testNo = 5;
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	client_list_counter = 0;
	client_list[client_list_counter++] = s1;
	result_counter = copy_clients_without(client_list_c, &client_list_counter_c, client_list, client_list_counter, s1);
	if (result_counter != 0){
		showFail(testNo);
		free(client_list);
		free(client_list_c);
		return;
	}
	free(client_list);
	free(client_list_c);
}
void _test_store_address_list(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_store_address_list:\n");
	int testNo = 0;
	int result_counter;
	
	/* local client list */
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0;
	/* outside client list */
	struct sockaddr_in *out_client_address_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(out_client_address_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int out_client_address_list_counter = 0;
	
	/* prepare addresses */
	struct sockaddr_in s0;
	target_server_address_build("localhost", 8888, &s0);
	struct sockaddr_in s1;
	target_server_address_build("localhost", 9999, &s1);
	struct sockaddr_in s2;
	target_server_address_build("localhost", 1000, &s2);
	struct sockaddr_in s3;
	target_server_address_build("localhost", 1500, &s3);
	struct sockaddr_in s11;
	target_server_address_build("localhost", 2000, &s11);
	
	/* load out_client_address_list */
	out_client_address_list[out_client_address_list_counter++] = s0;
	out_client_address_list[out_client_address_list_counter++] = s1;
	out_client_address_list[out_client_address_list_counter++] = s2;
	out_client_address_list[out_client_address_list_counter++] = s3;
	
	/* tests */
	testNo = 1;
	result_counter = store_address_list(out_client_address_list, out_client_address_list_counter, client_list, &client_list_counter);
	if (result_counter != 4){
		showFail(testNo);
	}
	testNo = 2;
	for (int i = 0; i < out_client_address_list_counter; i++){
		int same = address_is_same(client_list[i], out_client_address_list[i]);
		if (same != 1){
			showFail(testNo);
		}
	}
	
	testNo = 3;
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	client_list_counter = 0;
	client_list[client_list_counter++] = s11;
	
	result_counter = store_address_list(out_client_address_list, out_client_address_list_counter, client_list, &client_list_counter);
	if (result_counter != 5){
		showFail(testNo);
	}
	testNo = 4;
	for (int i = 0; i < out_client_address_list_counter; i++){
		int same = address_is_same(client_list[i+1], out_client_address_list[i]);
		if (same != 1){
			showFail(testNo);
		}
	}
	testNo = 5;
	int same = address_is_same(client_list[0], s11);
	if (same != 1){
		showFail(testNo);
	}
	
	
	free(client_list);
	free(out_client_address_list);
}

void _test_store_timestamp(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_store_timestamp:\n");
	int testNo = 0;
	
	struct timeval target_stamp; 
	bzero(&target_stamp,sizeof(struct timeval));
	target_stamp.tv_sec = 100;
	target_stamp.tv_usec = 500000;
	
	struct timeval time_stamp;
	testNo = 1;
	store_timestamp(&time_stamp, target_stamp);
	if (time_stamp.tv_sec != 100 || time_stamp.tv_usec != 500000){
		showFail(testNo);
	}
	
	testNo = 2;
	bzero(&target_stamp,sizeof(struct timeval));
	target_stamp.tv_sec = 0;
	target_stamp.tv_usec = 0;
	store_timestamp(&time_stamp, target_stamp);
	if (time_stamp.tv_sec != 0 || time_stamp.tv_usec != 0){
		showFail(testNo);
	}
	
	testNo = 3;
	bzero(&target_stamp,sizeof(struct timeval));
	target_stamp.tv_sec = 10086;
	target_stamp.tv_usec = 983432;
	store_timestamp(&time_stamp, target_stamp);
	if (time_stamp.tv_sec != 10086 || time_stamp.tv_usec != 983432){
		showFail(testNo);
	}
}

void _test_set_timeout_address(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_set_timeout_address:\n");
	int testNo = 0;
	int result = 2;
	
	char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	int portno = 9999;
	int parentfd = creat_socket();
	set_reuse_address(parentfd);
	struct sockaddr_in serveraddr; 
	struct sockaddr_in clientaddr; 
	bzero((char *) &clientaddr, sizeof(clientaddr));
	bzero((char *) &serveraddr, sizeof(serveraddr));
	int clientlen = sizeof(clientaddr);
	
	my_server_address_build(portno, &serveraddr);
	socket_port_bind(parentfd, &serveraddr);
	_test_parentfd_serverAddress_no_bind(parentfd, serveraddr);
	
	testNo = 1;
	result = set_timeout_address(parentfd, 1, 3);
	int len = recvfrom(parentfd, buf, BUFSIZE, 0,
					(struct sockaddr *) &clientaddr, &clientlen);
	result = set_timeout_address(parentfd, 0, 3);
	
	close(parentfd);
}

void _test_add_remove_WL(){
	if (!test_flag)
		return;
	showSep();
	printf("_test_add_remove_WL:\n");
	int testNo = 0;
	int result = 2;
	
	/* outside client list */
	struct sockaddr_in *out_client_address_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(out_client_address_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int out_client_address_list_counter = 0;
	
	/* prepare addresses */
	struct sockaddr_in s0;
	target_server_address_build("localhost", 8888, &s0);
	struct sockaddr_in s1;
	target_server_address_build("localhost", 9999, &s1);
	struct sockaddr_in s2;
	target_server_address_build("localhost", 1000, &s2);
	struct sockaddr_in s3;
	target_server_address_build("localhost", 1500, &s3);
	struct sockaddr_in s11;
	target_server_address_build("localhost", 2000, &s11);
	
	/* load out_client_address_list */
	out_client_address_list[out_client_address_list_counter++] = s0;
	out_client_address_list[out_client_address_list_counter++] = s1;
	out_client_address_list[out_client_address_list_counter++] = s2;
	out_client_address_list[out_client_address_list_counter++] = s3;
	
	
	
	/* prepare WL */
	struct WaitNode *wait_list = malloc(ARRAYSIZE * sizeof(struct WaitNode)); 
	bzero(wait_list, ARRAYSIZE * sizeof(struct WaitNode));
	int WL_counter = 0;
	
	/* test add_one_WL_node() */
	testNo = 1;
	result = add_one_WL_node(wait_list, &WL_counter, s0);
	if (result != 1 || WL_counter != 1){
		showFail(testNo);
	}
	
	testNo = 2;
	result = add_one_WL_node(wait_list, &WL_counter, s1);
	if (result != 1 || WL_counter != 2){
		showFail(testNo);
	}
	
	testNo = 3;
	result = add_one_WL_node(wait_list, &WL_counter, s2);
	if (result != 1 || WL_counter != 3){
		showFail(testNo);
	}
	
	testNo = 4;
	for (int i = 0; i < WL_counter; i++){
		if (address_is_same(out_client_address_list[i], wait_list[i].client) != 1){
			showFail(testNo);
		}
	}
	
	/* test remove_one_node_from_WL() */
	testNo = 5;
	result = remove_one_node_from_WL(wait_list, &WL_counter, s3);
	if (result != 0 || WL_counter != 3){
		showFail(testNo);
	}
	
	testNo = 5;
	result = remove_one_node_from_WL(wait_list, &WL_counter, s1);
	if (result != 1 || WL_counter != 2){
		showFail(testNo);
	}
	
	testNo = 6;
	for (int i = 0; i < WL_counter; i++){
		if (i != 1){
			int j = i;
			if (i == 2)
				j = 1;
			if (address_is_same(out_client_address_list[i], wait_list[j].client) != 1){
				showFail(testNo);
			}
		}
	}
	
	testNo = 7;
	bzero(wait_list, ARRAYSIZE * sizeof(struct WaitNode));
	WL_counter = 0;
	result = remove_one_node_from_WL(wait_list, &WL_counter, s0);
	if (result != 0 || WL_counter != 0){
		showFail(testNo);
	}
	
	free(wait_list);
	free(out_client_address_list);
}









