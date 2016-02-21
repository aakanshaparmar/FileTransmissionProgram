/**************************************************************
rdt-part3.h
Student name:Aakansha Parmar
Student No. :2012622238
Date and version:16th April 2015 Version 1
Development platform:Ubuntu 14.04
Development language:C
Compilation: Can be compiled with gcc
*****************************************************************/

#ifndef RDT3_H
#define RDT3_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define PAYLOAD 1000		//size of data payload of the RDT layer
#define TIMEOUT 50000		//50 milliseconds
#define TWAIT 10*TIMEOUT	//Each peer keeps an eye on the receiving  
							//end for TWAIT time units before closing
							//For retransmission of missing last ACK
#define W 5					//For Extended S&W - define pipeline window size


//----- Type defines ----------------------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word 

extern float LOSS_RATE, ERR_RATE;

u32b_t nextSeqNo=0;//Next sequence number to be sent
u32b_t S=0;;//First Sequence Number of Window
u32b_t N=0;;//Number of packets made

//ACK structure 
struct AckPack
{
	u8b_t pcktType; 	// Type of Packet A for Acknowledgement D for Data Packet
	u32b_t seqNo; 		// 0 or 1 depending on sequence of packet 
	u16b_t checksumValue; 	//Checksum value of packet
	u16b_t payLoadLen; 	//Lenght of Payload
	
} *ACKpkt; 

//Packet Structure
struct Packet 
{
	u8b_t pcktType; 	// Type of Packet A for Acknowledgement D for Data Packet
	u32b_t seqNo; 		// 0 or 1 depending on sequence of packet 
	u16b_t checksumValue; 	//Checksum value of packet
	u16b_t payLoadLen; 	//Lenght of Payload
	char payLoad[1000];		//Payload of data Packet
};

//Stucture to check if all packets in Window are Acked or not
struct Acked
{
	u32b_t seqNo;
	int ifAcked;

} window[W];




/* this function is for simulating packet loss or corruption in an unreliable channel */
/***
Assume we have registered the target peer address with the UDP socket by the connect()
function, udt_send() uses send() function (instead of sendto() function) to send 
a UDP datagram.
***/
int udt_send(int fd, void * pkt, int pktLen, unsigned int flags) {
	double randomNum = 0.0;

	/* simulate packet loss */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < LOSS_RATE){
		//simulate packet loss of unreliable send
		printf("WARNING: udt_send: Packet lost in unreliable layer!!!!!!\n");
		return pktLen;
	}

	/* simulate packet corruption */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < ERR_RATE){
		//clone the packet
		u8b_t errmsg[pktLen];
		memcpy(errmsg, pkt, pktLen);
		//change a char of the packet
		int position = rand() % pktLen;
		if (errmsg[position] > 1) errmsg[position] -= 2;
		else errmsg[position] = 254;
		printf("WARNING: udt_send: Packet corrupted in unreliable layer!!!!!!\n");
		return send(fd, errmsg, pktLen, 0);
	} else 	// transmit original packet
		return send(fd, pkt, pktLen, 0);
}

/* this function is for calculating the 16-bit checksum of a message */
/***
Source: UNIX Network Programming, Vol 1 (by W.R. Stevens et. al)
***/
u16b_t checksum(u8b_t *msg, u16b_t bytecount)
{
	u32b_t sum = 0;
	u16b_t * addr = (u16b_t *)msg;
	u16b_t word = 0;
	
	// add 16-bit by 16-bit
	while(bytecount > 1)
	{
		sum += *addr++;
		bytecount -= 2;
	}
	
	// Add left-over byte, if any
	if (bytecount > 0) {
		*(u8b_t *)(&word) = *(u8b_t *)addr;
		sum += word;
	}
	
	// Fold 32-bit sum to 16 bits
	while (sum>>16) 
		sum = (sum & 0xFFFF) + (sum >> 16);
	
	word = ~sum;
	
	return word;
}

//Funtion to make a packet 
Packet make_pkt(u32b_t seq, char* pcktData, u16b_t check, u8b_t type, int len)
{	
	//Set Values
	Packet sndpkt;
	sndpkt.pcktType=type;	
	sndpkt.seqNo=seq;
	sndpkt.checksumValue = check;
	memcpy(sndpkt.payLoad, pcktData, len);
	sndpkt.payLoadLen=len;
	sndpkt.checksumValue = checksum((u8b_t*)&sndpkt ,(u16b_t)(sizeof(struct AckPack)+len));	
	return sndpkt;
}

//Function to make ACK Packet 
void make_ACKpkt(u32b_t seq, u16b_t check, u8b_t type, int len)
{

	//Set Values
	ACKpkt= (AckPack*)malloc (sizeof(struct AckPack));
	ACKpkt->pcktType=type;	
	ACKpkt->seqNo=seq;
	ACKpkt->payLoadLen=0;
	ACKpkt->checksumValue= check;
	ACKpkt->checksumValue= checksum((u8b_t*)ACKpkt,(u16b_t)(sizeof(struct AckPack)));
}

//----- Type defines ----------------------------------------------------------

// define your data structures and global variables in here

 
u32b_t expectedSeqNo=0; //Sequence number of expected packet
int packetSize;//Size of packet being sent 

struct sockaddr_in peer_addr;
struct sockaddr_in my_addr;
struct hostent *host;
int sentByte;
socklen_t sin_size;

//Function to check if packet received is ACK 
int isACK(void* ACK)
{
	if(((AckPack *) ACK)->pcktType=='A')
		return 1;
	else 
		return 0;
}

//Function to check if packet received is Data Packet
int isData(void* Data)
{
	if(((Packet *)Data)->pcktType=='D')
		return 1;
	else 
		return 0;
}


//Function to check if corrupt
int isCorrupt(void* Pck, int len)
{
	
	return checksum((u8b_t*)Pck,(u16b_t)len);
}

//Function to return the number of packets that will be made
int countNoOfPackets (int length)
{
	
	if(length%1000 == 0)
		return length/1000;
	else
		return (length/1000)+1;
	
}


int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
   return	-> the socket descriptor on success, -1 on error 
*/
int rdt_socket() 
{
	int sockfd = socket (AF_INET, SOCK_DGRAM, 0);
	
	if(sockfd==-1)
	{
		perror("Unsuccessful\n");
		return -1;
	}
	else 
		return sockfd;
}

/* Application process calls this function to specify the IP address
   and port number used by itself and assigns them to the RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_bind(int fd, u16b_t port)
{
	my_addr.sin_family = AF_INET;         // host byte order	
    	my_addr.sin_port = htons(port);     // short, network byte order
    	my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically fill with my IP address
    	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

	/* Associate my address info to the socket */
    	if (bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) 
	{
        	perror("Unsuccessful\n");
        	return -1;
        }
	else 
		return 0;
}

/* Application process calls this function to specify the IP address
   and port number used by remote process and associates them to the 
   RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char * peer_name, u16b_t peer_port)
{
	host = gethostbyname(peer_name);   
 
      	peer_addr.sin_family = AF_INET;    // host byte order 
      	peer_addr.sin_port = htons(peer_port);  // short, network byte order 
      	peer_addr.sin_addr = *((struct in_addr *)host->h_addr); // already in network byte order
      	memset(&(peer_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

      	if (connect(fd, (struct sockaddr *)&peer_addr,  sizeof(struct sockaddr_in)) == -1)
	{
        	perror("Unsuccessful\n");
       		 return -1;
      	}
        else
        	return 0;
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket; this call will
   not return until the whole message has been successfully transmitted
   or when encountered errors.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length)
{
	AckPack recvPkt; //ACK Packet to be sent
	fd_set readfds;
	int sendResult;
	int totalSendResult=0;  
	
	//implement the Extended Stop-and-Wait ARQ logic
	//must use the udt_send() function to send data via the unreliable layer
	
	//Count number of packets that will be made
	N = countNoOfPackets(length);
	Packet *sndpkt[N];
	
	printf("Number of packet/s created - %d\n",N);
	S=nextSeqNo;
	int totalSize=length;
	int packetSize;
	
	//Make all packets if message size is greater than 
	for(u32b_t i=0;i<N;i++)
	{
		printf("Next Sequence Number- %d \n", nextSeqNo);
		if(totalSize>=PAYLOAD)
		{
			
			sndpkt[i]= (Packet*) malloc (sizeof(struct AckPack)+PAYLOAD);
			//Make Packet
			Packet temp=make_pkt (nextSeqNo, msg+(i*1000), 0, 'D', PAYLOAD);
			memcpy(sndpkt[i], &temp , sizeof(struct AckPack)+PAYLOAD);
			printf("Packet Made with sequence number %d and size %d\n", sndpkt[i]->seqNo, sndpkt[i]->payLoadLen);
			//exit(0);
			//Add to window
			window[i].seqNo=nextSeqNo;
			window[i].ifAcked=0;
			
			//Calculate packet size 
			packetSize = sizeof(struct AckPack) + sndpkt[i]->payLoadLen ; 

			//Send the message
			int sendResult = udt_send(fd, &sndpkt[i], packetSize , 0);
			totalSendResult += sendResult;
			printf("Packet of size - %d sent with sequence number - %d\n",sizeof(sendResult), nextSeqNo);
			totalSize-=PAYLOAD;
		}
		else 
		{
			sndpkt[i]= (Packet*)malloc (sizeof(struct AckPack)+totalSize);
			
			//Make Packet
			Packet temp=make_pkt (nextSeqNo, msg+(i*totalSize), 0, 'D', totalSize);
			memcpy(sndpkt[i], &temp, sizeof(struct AckPack)+totalSize);
			printf("Packet Made with sequence number %d and size %d\n", sndpkt[i]->seqNo, sndpkt[i]->payLoadLen);
			//exit(0);

			//Calculate packet size 
			packetSize = sizeof(struct AckPack) + sndpkt[i]->payLoadLen ; 
			
			window[i].seqNo=nextSeqNo;
			window[i].ifAcked=0;

			//Send the message
			sendResult = udt_send(fd, sndpkt[i], packetSize, 0);
			
			printf("Packet of size - %d sent with sequence number - %d\n",sendResult-sizeof(AckPack), nextSeqNo);
			
		}
	
		nextSeqNo++;
	}

	//Wait for ACK now

	int result;
	
	while(1)
	{
		
		//Set timeout values
		struct timeval timeout;
		timeout.tv_sec = 0;
  		timeout.tv_usec = TIMEOUT;

		//Reset file descriptors
		FD_ZERO(&readfds);
		FD_SET(fd,&readfds); 
  		
  		printf("Waiting for ACK...\n");
		//Call Select function() and Wait
  		result = select(fd+1,&readfds , NULL, NULL, &timeout);
  		
  		//In case of timeout 
		if(result == 0)
  		{
			printf("Timeout had occurred.\n");
			for(u32b_t i =0;i<N;i++)
			{ 
				//If packet is unacked resend it
				if(window[i].ifAcked==0)
				{
					udt_send(fd, sndpkt[i], sizeof(struct AckPack) + sndpkt[i]->payLoadLen, 0);
					printf("Packet with sequence number retransmitted - %d\n", sndpkt[i]->seqNo);
				}
		       }
		       timeout.tv_sec = 0;
  		       timeout.tv_usec = TIMEOUT;
		}

		//If a packet arrives
		else if(result==1)
		{
			if(FD_ISSET(fd,&readfds))
			{
				
				//Receive Packet
				int recvResult=recv(fd,(void*) &recvPkt, sizeof(struct AckPack), 0);

				//Check if ACK and is not corrupt and correct seq
				int chkACK = isACK( &recvPkt);
				int chkCor = isCorrupt ( &recvPkt, recvResult);
				

				//If it is an acknowledgement packet
				if(chkACK==1)
				{
					//If checksum is correct
					if(chkCor==0)
					{
						if(recvPkt.seqNo==S+N-1)
						{
							printf("All Sequence Numbers received\n");
							
							break;
							
						}
						else if (recvPkt.seqNo >= S && recvPkt.seqNo <= S+N-2)
						{
							printf("Receive Sequence No %d\n", recvPkt.seqNo);
							for(u32b_t i=0;i<N;i++)
							{
								if(window[i].seqNo <= recvPkt.seqNo)
								{
									window[i].ifAcked = 1;
								}
								else
								{	
								}
								

							} 
						}
						else
						{		
						timeout.tv_sec = 0;
  						timeout.tv_usec = TIMEOUT;}
					}  
					 	
					
					//If checksum is wrong
					else
					{
						timeout.tv_sec = 0;
  						timeout.tv_usec = TIMEOUT;
					}
				}

				//If it is a data packet 
				else 
				{
					//Check if Corrupt
			 		int chkCor = isCorrupt ( &recvPkt, recvResult);
					if(chkCor == 0)
					{
						//If it is an old retransmitted packet
						if(recvPkt.seqNo < expectedSeqNo)
						{
								
							printf("Sender got old retransmitted message sending ACK\n");
							
							//Make ACK Packet
							AckPack recvACK; 
							recvACK.seqNo=recvPkt.seqNo;
							recvACK.pcktType='A';
							recvACK.checksumValue= 0;
							recvACK.checksumValue= checksum((u8b_t*) &recvACK,(u16b_t)(sizeof(struct AckPack)));

							//Calculate packet size 
							int ACKpacketSize = sizeof(struct AckPack); 

							//Send acknowledgement
							udt_send(fd, &recvACK, ACKpacketSize, 0);
						}
						else
						{	
							timeout.tv_sec = 0;
  							timeout.tv_usec = TIMEOUT;
						}
							
					}
				}
			}
	      
		}
		 //If error occurs
		else if(result == -1)
		{
			perror("Error in select() fxn\n");
			exit(0);
		}
		

	} 


	for(u32b_t i=0;i<N;i++)
	   { free(sndpkt[i]);}
	
	
	
	return (totalSendResult - (N*sizeof(struct AckPack)));
	
}


/* Application process calls this function to wait for a message of any
   length from the remote process; the caller will be blocked waiting for
   the arrival of the message. 
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length)
{
	//implement the Stop-and-Wait ARQ (rdt3.0) logic
	//expectedSeqNo=0;
	int recvResult;
	Packet recvPkt;
	
	while(1)
	{	
		printf("Waiting for Packet\n");
		//Wait for packet to arrive 
		recvResult=recv(fd, (void *) &recvPkt, sizeof(AckPack)+PAYLOAD , 0);
		printf("Packet Arrived\n");
		//Check if data packet , has correct sequence, is not corrupt. 
		int chkData = isData( &recvPkt);
		int chkCor = isCorrupt ( &recvPkt, recvResult);
		
		//If packet received is a data packet 
		if(chkData ==1)
		{
			//If data packet is not corrupt and sequence number is correct
			if(chkCor==0)
			{
				if(recvPkt.seqNo == expectedSeqNo)
				{
						printf("Packet of Sequence Number- %d is not corrupt and has correct sequence\n", expectedSeqNo);
				
						//Make Acknowledgement Packet
						make_ACKpkt (expectedSeqNo, 0, 'A', length);

						//Calculate ACK size 
						int ACKpacketSize = sizeof(struct AckPack);

						//Send ACK
						int sendResult = udt_send(fd, ACKpkt, ACKpacketSize, 0);
						printf("Acknowledgement for sequence number- %d is sent of size- %d\n", expectedSeqNo, sendResult);
						
						//Increment expected sequence number
						expectedSeqNo++;
				
						//Return and Extract data from Packet
						memcpy(msg,recvPkt.payLoad,sizeof(recvPkt.payLoad)); 

						break;
				}
				else 
				{
					printf("Wrong sequence packet received");

					//Resend acknowledgement of past 
				
				
					//Make Acknowledgement Packet of older packet
					make_ACKpkt (expectedSeqNo-1, 0, 'A', length);

					//Calculate ACK size 
					int ACKpacketSize = sizeof(struct AckPack);

					//Send ACK
					int sendResult = udt_send(fd, ACKpkt, ACKpacketSize, 0);
					printf("Previous acknowledgement for sequence number- %d is sent of size- %d\n", expectedSeqNo-1, sendResult);	
				}

			}
			
			// If Data packet is corrupt or wrong sequence number 
			else 
			{
				printf("Corrupted packet received");
				continue;
			}
		}
		//If ACK Packet Arrives
		else 
		{ 	printf("Receiver got ACK packet ignoring it\n");
			continue;
		}

	}
	
	
	return (recvResult-sizeof(AckPack)); 

}


/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd)
{
	//implement the Extended Stop-and-Wait ARQ logic
	//implement the Stop-and-Wait ARQ (rdt3.0) logic

	int result;
	fd_set readfds; 
	Packet recvPkt;

	while(1)
	{
		
  		struct timeval timeout;
		timeout.tv_sec = 0;
  		timeout.tv_usec = TWAIT;
		FD_ZERO(&readfds);//reset 
		FD_SET(fd,&readfds);

		//Wait for last packet to come 
		result = select(fd+1,&readfds , NULL, NULL, &timeout);
  		
		//If timeout happens end program
		if(result==0)
  		{
  			break;
  		}
  		
		//If Packet arrives 
		else if(result==1)
  		{
  			if(FD_ISSET(fd,&readfds))
			{
				
				//In case packet is received.
				int recvResult=recv(fd,(void*) &recvPkt, sizeof(struct Packet), 0);
				printf("Received a packet\n");
				int chkData = isData( &recvPkt);
				int chkCor = isCorrupt ( &recvPkt, recvResult);
				
				
				if((chkData==1) && (chkCor==0))
				{	
					if(recvPkt.seqNo < expectedSeqNo)
					{
						printf("Last packet received is Data packet and is not corrupt\n");
						int ACKpacketSize = sizeof(struct AckPack); 
						make_ACKpkt (recvPkt.seqNo, 0, 'A', ACKpacketSize);

						//Send acknowledgement
						int sendResult = udt_send(fd, ACKpkt, ACKpacketSize, 0);
						printf("Last ACK size- %d sent\n", sendResult);
						timeout.tv_sec = 0;
  						timeout.tv_usec = TWAIT;
					}	

				}
			    
			 }                                                     
				
				
			
		}
		else 
		{
			perror("Error in select() fxn\n");
			exit(0);
		}

	}
	
	free(ACKpkt);
	printf("Socket Closed\n");
	return close(fd);
}

#endif
