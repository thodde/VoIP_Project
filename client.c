/* 
 * client.c
 * Author: Trevor Hodde
 * Calls a server named as the first argument
 * to communicate with over a specified port
 * and protocol.
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
#define ERROR_RATE 0       //dropped bits percentage

#define LENGTH 1    //how many seconds
#define CHANNELS 1  //mono

#define BUF_SIZE (LENGTH*SAMPLE_RATE*SAMPLE_SIZE*CHANNELS/8)

//these will be used to store audio data
int audio_fd;
unsigned char audio_buffer[BUF_SIZE];

//these will be assigned based on command line args
int port;
char* serv_host_addr;

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff /* should be in <netinet/in.h> */
#endif

//function prototypes
int tcp_connection();
int udp_connection();

int main(int argc, char *argv[]) {
   //used for storing the protocol
   char protocol[3];  
   int error_rate = ERROR_RATE;
   int sample_rate = SAMPLE_RATE;
   int sample_size = SAMPLE_SIZE;
   int new_buf_size;

   printf("%s\n\n", "");
   printf("%s\n", "-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-");
   printf("%s\n", "\t\tWelcome to Speak: Client Side!");
   printf("%s\n", "\tA Simple VoIP Application by Trevor Hodde");
   printf("%s\n", "-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-");
   printf("%s\n", ""); 

   //make sure the correct number of arguments are specified
   if (argc < 4 || argc > 7) {
      fprintf(stderr, "\tusage: %s <host> <port> <protocol> <error percentage> <sample rate> <sample size>\n", argv[0]);
      fprintf(stderr, "\t<host>\t\t\t- Internet name of server host\n");
      fprintf(stderr, "\t<port>\t\t\t- port\n");
	  fprintf(stderr, "\t<protocol>\t\t- either TCP or UDP\n");
	  fprintf(stderr, "\t<error percentage>\t- optional packet drop rate [1-100 default: 0]\n");
	  fprintf(stderr, "\t<sample rate>\t\t- optional sample rate [default: 8000]\n");
	  fprintf(stderr, "\t<sample size>\t\t- optional sample size [default: 8]\n");
	  fprintf(stderr, "NOTE: Please use the same protocol, sample rate and sample size as the server and other clients\n");
	  fprintf(stderr, "... Just to make life easier.\n");
      exit(1);
   }
   
   // sets the hostname and the port to communicate with
   serv_host_addr = argv[1];
   port = atoi(argv[2]);
   
   //set the dropped packets percentage [default: 0]
   if(argv[4] != NULL) {
      error_rate = atoi(argv[4]);
   }
   
   //make sure there are actual arguments there to avoid memory leaks
   if(argc > 4) {
	   //set the sample rate [default: 8000]
	   if(argv[5] != NULL) {
		  sample_rate = atoi(argv[5]);
	   }
	   
	   //set the sample size [default: 8]
	   if(argv[6] != NULL) {
		  sample_size = atoi(argv[6]);
	   }
	}

	//if all the delay parameters worked, this would increase the size of the audio buffer
	new_buf_size = (LENGTH*sample_rate*sample_size*CHANNELS/8);
   
   // Determine which protocol is going to be used (TCP by default)
   if(argv[3] == NULL || (strcmp(argv[3], "TCP") == 0)) {
      strcpy(protocol, "TCP");
      tcp_connection();
   } 
   else if(strcmp(argv[3], "UDP") == 0) {
      strcpy(protocol, "UDP");
      udp_connection();
   }
   else {
      fprintf(stderr, "\tusage: %s <host> <port> <protocol> <error percentage> <sample rate> <sample size>\n", argv[0]);
      fprintf(stderr, "\t<host>\t- Internet name of server host\n");
      fprintf(stderr, "\t<port>\t- port\n");
	  fprintf(stderr, "\t<protocol>\t- either TCP or UDP\n");
	  fprintf(stderr, "\t<error percentage>\t- optional packet drop rate [1-100 default: 0]\n");
	  fprintf(stderr, "\t<sample rate>\t- optional sample rate [default: 8000]\n");
	  fprintf(stderr, "\t<sample size>\t- optional sample size [default: 8]\n");
	  fprintf(stderr, "NOTE: Please use the same protocol, sample rate and sample size as the server and other clients\n");
	  fprintf(stderr, "... Just to make life easier.\n");
      exit(1);
   }
}

/**
 * This function creates a TCP socket to send sound
 * packets to the server through.
 */
int tcp_connection() {
   unsigned long int inaddr;
   int sock;
   struct sockaddr_in serv_addr;
   struct hostent *hp;
   int status;
   printf("Talk activated.\n\n");
   printf("Trying to connect to server %s at port %d...\n", serv_host_addr, port);

   /*
    * First try to convert the host name as a dotted-decimal number.
    * Only if that fails do we call gethostbyname().
    */
   bzero((void *) &serv_addr, sizeof(serv_addr));
   printf("Looking up %s...\n", serv_host_addr);
   
   //make sure the hostname is not null
   if ((hp = gethostbyname(serv_host_addr)) == NULL) {
     perror("host name error");
     exit(1);
   }
   
   bcopy(hp->h_addr, (char *) &serv_addr.sin_addr, hp->h_length);

   //establish a connection with the port
   printf("Found it.  Setting port connection to %d...\n", port);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(port);

   /* open a TCP socket */
   puts("Done. Opening socket...");
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
     perror("opening socket");
     exit(1);
   }

   /* socket open, so connect to the server */
   puts("Open. Trying connection to server...");
   if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
     perror("can't connect");
     exit(1);
   }

   printf("Connection established! \n\n");

   //open the audio device [/dev/dsp -- because I am using cygwin]
   if ((audio_fd = open("/dev/dsp", O_RDWR)) == -1) {
      perror("/dev/dsp");
      exit(1);
   }

   printf("Audio device opened! \n\n");
   
   //continue until the user presses CTRL+c to quit
   printf("say something...\n");
   while(1) {
	  //read audio from the device
	  status = read(audio_fd, audio_buffer, sizeof(audio_buffer));
      
	  //make sure the correct number of bytes were recorded
	  if(status != sizeof(audio_buffer)) { 
		perror("wrong number of bytes");
	  }
	  status = ioctl(audio_fd, SOUND_PCM_SYNC, 0);
	  
	  //send the recorded data to the server
	  send(sock, audio_buffer, sizeof(audio_buffer), 0);
   } 
   
   close(sock);

   exit(0);
}

/**
 * This function creates a UDP socket to send sound
 * packets to the server through.
 */
int udp_connection() {
   int i, sock, bytes;
   char buf[80];
   struct sockaddr_in  srv_addr;  /* server's Internet socket address */
   struct hostent      *hp; /* from gethostbyname() */
   int status;

   printf("Talk activated over UDP protocol.\n\n");
   printf("Trying to connect to server %s at port %d...\n", serv_host_addr, port);

   //get the IP address or hostname
   bzero((char *) &srv_addr, sizeof(srv_addr));
   srv_addr.sin_family = AF_INET;
   srv_addr.sin_port = htons(port);
   if ((hp = gethostbyname(serv_host_addr)) == NULL) {
     perror("host name error");
     exit(1);
   }
  
   bcopy(hp->h_addr, (char *) &srv_addr.sin_addr, hp->h_length);

   //open the udp socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
     perror("opening udp socket");
     exit(1);
   }

   //connect to the socket
   if (connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr))<0) {
     perror("connect error");
     perror("Make sure you are using the same protocol that the server is running on!");
     exit(1);
   }
   
   printf("Connection established! \n\n");
   
   //open the audio device [/dev/dsp -- because I am using cygwin]
   if ((audio_fd = open("/dev/dsp", O_RDWR)) == -1) {
      perror("/dev/dsp");
      exit(1);
   }

   printf("Audio device opened! \n\n");

   //continue until the user presses CTRL+c to quit
   printf("say something...\n");
   while (1) {
     //read audio from the device
	  status = read(audio_fd, audio_buffer, sizeof(audio_buffer));
      
	  //make sure the correct number of bytes were recorded
	  if(status != sizeof(audio_buffer)) { 
		perror("wrong number of bytes");
	  }
	  status = ioctl(audio_fd, SOUND_PCM_SYNC, 0);
	  
	  //send the recorded data to the server
	  sendto(sock, audio_buffer, sizeof(audio_buffer), 0, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
   }

   close(sock);
}
