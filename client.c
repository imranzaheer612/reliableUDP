#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>	
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>


#define VIDEO_FILE "testFiles/sampleVideo.mp4"
#define PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define BUFFER_SIZE 500
#define WINDOW_SIZE 5


/**
 * defining structure of packet
 * and Acks
*/

struct Packet {
	int seqNum;
	int size;
	char data[BUFFER_SIZE];
	bool sent;
	bool received;
	bool ack;
	bool eof;
};

struct ackPacket
{
   int seqNum;
   int size;
   bool eof;
};



/**
 * initializing variables
*/
FILE *fp;
int udpSocket;
struct sockaddr_in server;
int addrlen = sizeof(server);
char buffer[BUFFER_SIZE];
int num_of_ack_recieved = 0;

int ack_of_eof = 0;
int acks_to_wait = WINDOW_SIZE - 1;
int n;
int seq_num = 0;
bool eof_reached;
int eof_set = 0;

struct Packet window[WINDOW_SIZE];

/**
 * Initializing functions
*/
void sendOverR_UDP();
void sendOverUDP();
void thread_waiting();
void wait_ack();
void addPacket(struct Packet newPacket);
void slideWindow(int steps);



/**
 * initializing socket connection
*/
int socketConnection() {

   udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

   if (udpSocket < 0) {
      perror("[-]Couldn't creat socket!");
   }

   // server config
   server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
   server.sin_family = AF_INET;
   server.sin_port = htons(PORT);
}

/**
 * Reliable udp sender
*/
void sendOverR_UDP() {
   acks_to_wait = WINDOW_SIZE;

   RESEND:
   sendOverUDP();
   wait_ack();

   // if all acks not received send again
   if (num_of_ack_recieved < acks_to_wait) {
      goto RESEND;
   }
   
   // if eof then stop
   if (eof_reached == false) {
      slideWindow(WINDOW_SIZE);
      sendOverR_UDP();
   }
}


/**
 * sending the data over udp socket
*/

void sendOverUDP() {
   for (int i = 0; i < WINDOW_SIZE; i++) {
      if (window[i].ack == false){

         n = sendto(udpSocket, &window[i], sizeof(window[i]), MSG_CONFIRM, (const struct sockaddr *) &server, addrlen);

         printf("packet sent: %d\n", window[i].seqNum);

         if (window[i].size == 0) // end of file.
            break;

         else if (n == 0)
            break;
      }
   }
}


/**
 * waiting for the acks of the
 * packets that has been sent
*/
void wait_ack(){
   int n;
   for (int i = 0; i < acks_to_wait; i++)
   {
      wait_for_ack: ;
      struct ackPacket newAck;
      n = recvfrom(udpSocket, &newAck, sizeof(&newAck), 0,  (struct sockaddr *) &server, &addrlen);
      if (n < 0) 
            perror("ERROR reading from socket");

      int ack_num = newAck.seqNum;

      // matching ack with expected ack
      for (int i = 0; i < WINDOW_SIZE; i++)
      {
         if (window[i].seqNum == ack_num) {

            // if already acked
            if (window[i].ack == true) {
               goto wait_for_ack;
            }
            else {
               window[i].ack = true;
               // printf("eof: %d\n", newAck.eof);
               printf("ack received: %d\n", ack_num);

               //if eof reached
               if (eof_set > 0) {
                  newAck.seqNum = eof_set;
                  printf("eof reached");
                  eof_reached = true;
                  acks_to_wait = newAck.seqNum+1;
               }
               num_of_ack_recieved++;
            }
         }
      }
   }
}

/**
 * pushing packet to the array
*/
void addPacket(struct Packet newPacket) {
   /* shifting array elements */
   n = WINDOW_SIZE;
   for(int i=0; i < n-1; i++){
      window[i] = window[i+1];
   }
   window[n-1] = newPacket;
}


/**
 * slides the window
 * --> add new pkts
 * --> and fill them with new data
*/
void slideWindow(int steps) {

   for (int i = 0; i < steps; i++)
   {
      struct Packet packet;
      size_t num_read = fread(packet.data, 1, BUFFER_SIZE, fp);

      packet.seqNum = seq_num;   
      packet.size = num_read;
      packet.sent = false;
      packet.ack = false;

      if (num_read == 0) {
         printf("eof pkt: %d\n", packet.seqNum);
         packet.eof = true;
         eof_set = packet.seqNum;
         addPacket(packet);
         break;
      }

      addPacket(packet);
      seq_num++;

      if (seq_num >= WINDOW_SIZE) {
         seq_num = 0;
      }
   }
}

/**
 * 
 * TRANSFER FILE OVER UDP
 * 
*/
int main(int argc , char *argv[]) {   

   //set timeout on the socket receiving end
   struct timeval timeout;      
   timeout.tv_sec = 0;
   timeout.tv_usec = 100000;
   socketConnection();
   if (setsockopt (udpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
        printf("setsockopt failed\n");
   
   fp = fopen(VIDEO_FILE, "rb"); 
   if (fp == NULL) 
      printf("[-]File open failed!\n"); 
   else
      printf("[+]File successfully opened!\n");
   puts("[+]Sending file.");

   // ready the first packets
   slideWindow(5);

   // then start transfer
   sendOverR_UDP();

   printf("[+]File sending complete...\n");
   if (fp != NULL)  
      fclose(fp);
   close(udpSocket);

   return 0;
}
