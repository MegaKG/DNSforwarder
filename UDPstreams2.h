#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//The Basic Connection type
struct connection {
    struct sockaddr_in     addr;
    int sockfd;
};

//Open a UDP Client Connection
struct connection* newClient(char* IP, int PORT){
    //Socket Vars
    int sockfd;
    struct sockaddr_in     servaddr;
   
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    //Clear the Address with Zeros
    memset(&servaddr, 0, sizeof(servaddr));
       
    //Specity Address Type
    servaddr.sin_family = AF_INET;
    
    //Specify IP
    servaddr.sin_addr.s_addr = inet_addr(IP);

    //Specify Port
    servaddr.sin_port = htons(PORT);

    //Generate the Object
    struct connection* Output = (struct connection*)malloc(sizeof(struct connection));
    Output->addr = servaddr;
    Output->sockfd = sockfd;

    return Output; 
}



//Host a UDP Server Connection
struct connection* newServer(char* IP, int PORT){
    //Socket Vars
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
       
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
       
    //Clear the Addresses
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
       
    //Specity Address Type
    servaddr.sin_family = AF_INET;
    
    //Specify IP
    servaddr.sin_addr.s_addr = inet_addr(IP);

    //Specify Port
    servaddr.sin_port = htons(PORT);

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");


    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
        perror("setsockopt(SO_REUSEPORT) failed");

       
    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  sizeof(servaddr)) < 0 ){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //Generate the Object
    struct connection* Output = (struct connection*)malloc(sizeof(struct connection));
    Output->addr = cliaddr;
    Output->sockfd = sockfd;

    return Output; 

}

//Read a Packet
int getdat(char Buffer[],int buffsize,struct connection* con){
    int got;
    int len = sizeof(con->addr);
    got = recvfrom(con->sockfd, (char *)Buffer, buffsize, MSG_WAITALL, (struct sockaddr *) &con->addr, (socklen_t*)&len);

    Buffer[got] = 0;
    return got;
}

//Write a Packet
void senddat(char* Data,int Len,struct connection* con){
    int len = sizeof(con->addr);
    sendto(con->sockfd, (const char *)Data, Len, MSG_CONFIRM, (const struct sockaddr *) &con->addr, len);
}

//Clean up a connection
void closeCon(struct connection* con){
    close(con->sockfd);
}