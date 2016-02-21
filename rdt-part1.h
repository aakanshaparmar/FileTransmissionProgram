/**************************************************************
rdt-part1.h
Student name:	Aakansha Parmar
Student No. :	2012622238
Date and version:	2 March 2015 Version 1
Development platform:	Ubunut 14.04
Development language:	C
Compilation: Can be compiled with gcc compiler
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word 

struct sockaddr_in peer_addr;
struct sockaddr_in my_addr;
struct hostent *host;
 int sentByte;
socklen_t sin_size;

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
		perror("Unsuccessful");
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
        	perror("Unsuccessful");
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
        	perror("Unsuccessful");
       		 return -1;
      	}
        else
        	return 0;	
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length)
{   
      
      if ((sentByte=sendto(fd, msg, length, 0, (struct sockaddr *)&peer_addr,  sizeof(struct sockaddr_in))) == -1)
      {
        	perror("sent to");
        	return -1;
      }
      else 
		return sentByte;
		
}

/* Application process calls this function to wait for a message 
   from the remote process; the caller will be blocked waiting for
   the arrival of the message.
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length)
{
      sin_size = sizeof(struct sockaddr_in); 
      int numbytes;
      

       if ((numbytes=recvfrom(fd, msg, length, 0, (struct sockaddr *)&peer_addr, &sin_size)) == -1) 
       {
        	perror("received from");
        	return -1;
       }   
       else
      		return numbytes;
	
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd)
{
     close(fd);
     return 0;
}

#endif
