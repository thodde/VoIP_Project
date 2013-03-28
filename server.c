/* 
 * server.c
 * Author: Trevor Hodde
 * Listens for client connections on a
 * specified port through a specified protocol.
 *
 * Some code borrowed from http://web.cs.wpi.edu/~cs529/s13/projects/proj2/index.html
 * This includes TCP and UDP socket setup on both client and server side 
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>

//These are defaults, but can be passed in as command line args
#define SAMPLE_RATE 8000   //sample rate
#define SAMPLE_SIZE 8      //sample size [bits]

#define LENGTH 1   //how many seconds
#define CHANNELS 1  //mono

#define BUF_SIZE (LENGTH*SAMPLE_RATE*SAMPLE_SIZE*CHANNELS/8)

int audio_fd;
unsigned char audio_buffer[BUF_SIZE];
int port;

#define MAX_CLIENTS 2

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff /* should be in <netinet/in.h> */
#endif

//function prototypes
int tcp_connection();
void process_data_tcp(int, int[], int);
int udp_connection();

int main(int argc, char *argv[]) {
   char protocol[3];
   int sample_rate = SAMPLE_RATE;
   int sample_size = SAMPLE_SIZE;
   int new_buf_size;

   printf("%s\n\n", "");
   printf("%s\n", "-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-");
   printf("%s\n", "\t\tWelcome to Speak: Server Side!");
   printf("%s\n", "\tA Simple VoIP Application by Trevor Hodde");
   printf("%s\n", "-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-");
   printf("%s\n\n", "");

   //make sure the server is given all the info it needs when it is executed
   if (argc < 3 || argc > 5) {
      fprintf(stderr, "\tusage: %s <port> <protocol> <sample size> <sample rate>\n", argv[0]);
      fprintf(stderr, "\t<port>\t- port to listen on\n");
	  fprintf(stderr, "\t<protocol>\t - either TCP or UDP\n");
	  fprintf(stderr, "\t<sample rate>\t\t- optional sample rate [default: 8000]\n");
	  fprintf(stderr, "\t<sample size>\t\t- optional sample size [default: 8]\n");
	  fprintf(stderr, "NOTE: Please use the same sample rate and sample size as the clients\n");
	  fprintf(stderr, "... Just to make life easier.\n");
      exit(1);
   } 
   
   //assign a value to the port
   port = atoi(argv[1]);
   
   //make sure there are actual arguments there to avoid memory leaks
   if(argc > 3) {
	   //set the sample rate [default: 8000]
	   if(argv[4] != NULL) {
		  sample_rate = atoi(argv[4]);
	   }
	   
	   //set the sample size [default: 8]
	   if(argv[5] != NULL) {
		  sample_size = atoi(argv[5]);
	   }
	}

	//if all the delay parameters worked, this would increase the size of the audio buffer
	new_buf_size = (LENGTH*sample_rate*sample_size*CHANNELS/8);

	//determine which protocol is to be used
    if(argv[2] != NULL && (strcmp(argv[2], "TCP") == 0)) {
	  strcpy(protocol, "TCP");
      tcp_connection();
	}
	else if(argv[2] != NULL && (strcmp(argv[2], "UDP") == 0)) {
	  strcpy(protocol, "UDP");
      udp_connection();
	}
	else {
      fprintf(stderr, "\tusage: %s <port> <protocol> <sample size> <sample rate>\n", argv[0]);
      fprintf(stderr, "\t<port>\t- port to listen on\n");
	  fprintf(stderr, "\t<protocol>\t - either TCP or UDP\n");
	  fprintf(stderr, "\t<sample rate>\t\t- optional sample rate [default: 8000]\n");
	  fprintf(stderr, "\t<sample size>\t\t- optional sample size [default: 8]\n");
	  fprintf(stderr, "NOTE: Please use the same sample rate and sample size as the clients\n");
	  fprintf(stderr, "... Just to make life easier.\n");
      exit(1);
   }
}

/**
 * This function listens for tcp connections and sets up sockets
 * when a client connects. It spawns a new process to keep the main
 * process available when a client connects.
 */
int tcp_connection() {
   int sock, clilen, newsock;
   struct sockaddr_in cli_addr, serv_addr;
   pid_t pid;
   int sockets[MAX_CLIENTS]; //an array for storing the sockets through which the different clients will communicate
   int number_of_clients = 0;

   printf("Listen activating.\n");
   printf("Trying to create socket at port %d...\n", port);   

   /* Create socket from which to read */
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("can't open stream socket");
      exit(1);
   }
   
   /* bind our local address so client can connect to us */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons(port);
   
   if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("can't bind to local address");
      exit(1);
   }

   printf("Socket created! Accepting connections.\n\n");

   //listen for 2 client connections
   listen(sock, 2);

   clilen = sizeof(cli_addr);
   
   //keep listening for new connections while the server is running
   while (1) {
	  //accept a new client and create a new socket
      newsock = accept(sock, (struct sockaddr *) &cli_addr, &clilen);
	  
	  if(number_of_clients < MAX_CLIENTS) {
		  if (newsock < 0) {
			 perror("accepting connection");
			 exit(1);
		  }
		  
		  /* Create child process */
		  pid = fork();
		  if (pid < 0) {
			 perror("ERROR on fork");
			 exit(1);
		  }

		  if (pid == 0) {
			  /* This is the client process */
			  close(sock);

			  sockets[number_of_clients] = newsock;
			  number_of_clients++;
			  //let child process handle the new client
			  process_data_tcp(newsock, sockets, number_of_clients);
			  exit(0);
		  }
		  else {
			  close(newsock);
		  }
	    }
   }
   
   return 0;
}

/**
 * This function receives the audio data
 * from the client and plays sound until it
 * is finished.
 */
void process_data_tcp(int sock, int sockets[], int number_of_clients) {
   int i;
   
   //make sure the socket is valid before using it
   if (sock < 0) {
     perror("accepting connection");
     exit(1);
   }
   
   //open the audio device to play the sound
   if ((audio_fd = open("/dev/dsp", O_RDWR)) == -1) {
      perror("/dev/dsp");
      exit(1);
   }
   
   //continue playing sounds as they arrive and clear the buffer when
   //the sound finishes to make room for the next sound
   printf("Listening...\n");
   while (1) {
     bzero(audio_buffer, sizeof(audio_buffer));
	 recv(sock, audio_buffer, sizeof(audio_buffer), 0);
	 write(audio_fd, audio_buffer, sizeof(audio_buffer));
   }
}

/**
 * This function listens for tcp connections and sets up sockets
 * when a client connects. It spawns a new process to keep the main
 * process available when a client connects.
 */
int udp_connection() {
   int sock;
   struct sockaddr_in name;
   struct hostent *hp, *gethostbyname();
   socklen_t fromlen;
   int bytes;

   printf("Listen activating UDP connection.\n");

   /* Create socket from which to read */
   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0)   {
     perror("Opening datagram socket");
     exit(1);
   }
  
   /* Bind our local address so that the client can send to us */
   bzero((char *) &name, sizeof(name));
   name.sin_family = AF_INET;
   name.sin_port = htons(port);
   name.sin_addr.s_addr = htonl(INADDR_ANY);
  
  //make sure the sock is set up correctly
   if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
     perror("binding datagram socket");
     exit(1);
   }
  
   printf("Socket has port number #%d\n", ntohs(name.sin_port));
   
   //open the audio device to play the sound
   if ((audio_fd = open("/dev/dsp", O_RDWR)) == -1) {
      perror("/dev/dsp");
      exit(1);
   }
  
   //as long as the client is connected, receive audio data and play it back as it is received
   printf("Listening...\n");
   while (1) {
     bzero(audio_buffer, sizeof(audio_buffer));
	 fromlen = sizeof(name);
	 recvfrom(sock, audio_buffer, sizeof(audio_buffer), 0, (struct sockaddr*)&name, &fromlen);
	 write(audio_fd, audio_buffer, sizeof(audio_buffer));
   }

   close(sock);

   return 0;
}
