// GPL3 License, Code Written by MegaKG
//Compile Command:
//g++ KDNSserverThreaded.cpp -pthread -o DNSserver.exe

//Configuration
const char* HostAddress = "127.0.0.53";
const char* UpstreamAddress = "1.1.1.1";
const int bufsize = 1024;
const char ParseQuestion = 1;

//Program Begins Here
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "UDPstreams2.h"
#include <vector>
#include <pthread.h>
using namespace std;



//Converts Bytes to Int
union ui16 {
    uint16_t TheInt;
    char TheBuf[2];
};

//Endian Param:
//0 = Normal, 1 = Flip
//Will Copy Buffer to Second Buffer
void bcpy(int start, int end, char Orig[], char En[],char Endian){
    int st = start;
    unsigned char tm;
    for (int i = 0; i < (end-start); i++){
        if (Endian){
            tm = Orig[end-1-st];
        }   
        else {
            tm = Orig[st];
        }
        
        En[i] = tm;
        st++;
    }
}

//Dumps a buffer of Length in Hex
void hexdump(char* In, int Length){
    unsigned char b;
    for (int i = 0; i < Length; i++){
        b = In[i];
        printf("%i ",b);
    }
    printf("\n");
}

//Clears a Buffer of Length
void clearBuf(char In[],int Length){
    for (int i = 0; i < Length; i++){
        In[i] = 0;
    }
}

//Contains arguments for threads
struct iarguments {
    struct connection* ClientCon;
    int Length;
    char Buffer[bufsize];
};


void* handlerequest(void* args){
    //Deconstruct the Input Arguments
    struct iarguments* MyArgs = (struct iarguments*)args;
    struct connection* ClientCon = MyArgs->ClientCon;
    int Length = MyArgs->Length;
    char Request[bufsize];
    bcpy(0,bufsize,MyArgs->Buffer, Request, 0);

    //detach the current thread
    //from the calling thread
    pthread_detach(pthread_self());

    //Dump the Packet as Hex
    hexdump(Request,Length);

    //Connect to the DNS Server
    struct connection* ServerCon = newClient((char*)UpstreamAddress,53);
    printf("Upstream Socket Created\n");

    //Initialise the Converter from bytes to integers
    union ui16 conv;

    //Read ID
    conv.TheBuf[0] = Request[1];
    conv.TheBuf[1] = Request[0];
    uint16_t ID = conv.TheInt;

    //Read Opcode
    char Opcode = (120 & Request[2]) >> 3;

    //Read the Flags
    char QR, AA, TC, RD, RA, Z, Rcode;
    QR = (128 & Request[2]) != 0;
    AA = (4 & Request[2]) != 0;
    TC = (2 & Request[2]) != 0;
    RD = (1 & Request[2]) != 0;
    RA = (128 & Request[3]) != 0;
    Z  = (112 & Request[3]) >> 4;
    Rcode = (15 & Request[3]) != 0;

    //Questions
    conv.TheBuf[0] = Request[5];
    conv.TheBuf[1] = Request[4];
    uint16_t Questions = conv.TheInt;

    //Answers
    conv.TheBuf[0] = Request[7];
    conv.TheBuf[1] = Request[6];
    uint16_t Answers = conv.TheInt;

    //Nameserver
    conv.TheBuf[0] = Request[9];
    conv.TheBuf[1] = Request[8];
    uint16_t Nameserver = conv.TheInt;

    //Additional
    conv.TheBuf[0] = Request[11];
    conv.TheBuf[1] = Request[10];
    uint16_t Additional = conv.TheInt;

    //Optionally Parse the question
    if (ParseQuestion){
        //Read the Question Names
        //This stores whole DNS queries
        vector<char*> QuestionData;
        //This stores individual parts of a question. This is then processed and appended as a C_string to Question Data
        vector<char> QuestionBuf;

        //Dump the Request
        printf("Requested ID %i Len %i Opcode %i QR %i AA %i TC %i RD %i RA %i Z %i Rcode %i Questions %i Answers %i Additional %i Nameservers %i\n",
            ID, Length, Opcode, QR, AA, TC, RD, RA, Z, Rcode, Questions, Answers, Additional, Nameserver );


        //Parse the Question
        char* FullURL;
        //Start after the header
        int position = 12;
        //The 8 Bit length of the Question
        uint8_t Qlength = 0;
        //The Total Characters Read
        int Read = 0;

        printf("Questions %i to Read\n",Questions);
        for (int i = 0; i < Questions; i++){
            //Clear all parts from the temporary buffer
            QuestionBuf.clear();
            Read = 0;

            //Iterate over all Questions
            while (true){
                //Get the Length of the Question (Hostname Fragment)
                Qlength = Request[position];
                printf("Read Question Length of %i\n",Qlength);

                Read += (Qlength + 1);
                position += 1;
                if (Qlength == 0){
                    break;
                }
                else {
                    for (int l = 0; l < Qlength; l++){
                        //Read the Question and append it to QuestionBuf
                        printf("%c",Request[position]);
                        QuestionBuf.push_back(Request[position]);
                        position += 1;
                    }
                    printf("\n");
                    QuestionBuf.push_back('.');
                }

            }
            //Now add the complete hostname to the QuestionData array
            Read -= 1;
            
            printf("Building Full URL, Read in Total %i\n",Read);
            FullURL = (char*)malloc(sizeof(char)*Read);
            for (int l = 0; l < Read; l++){
                FullURL[l] = QuestionBuf[l];
                printf("%c",FullURL[l]);
            }
            printf("\n");
            FullURL[Read] = 0;

            QuestionData.push_back(FullURL);
        }
        printf("Loaded Questions\n");


        //Dump the contents
        for (int i = 0; i < QuestionData.size(); i++){
            printf("%i %s\n",ID,QuestionData[i]);
        }

        //Clean up our memory
        for (int i = 0; i < QuestionData.size(); i++){
            free(QuestionData[i]);
        }
        QuestionData.clear();

    }
    
    //Forward Request
    senddat(Request,Length,ServerCon);

    //Get Response from the Upstream Server
    clearBuf(Request,bufsize);
    Length = getdat(Request,bufsize,ServerCon);
    printf("Got Response\n");

    //Reply
    senddat(Request,Length,ClientCon);
    printf("Replied\n");

    


    //Clean up our sockets
    closeCon(ServerCon);
    closeCon(ClientCon);
    printf("Closed Up\n");

    // exit the current thread
    pthread_exit(NULL);

}




int main(){
    //Initialise the DNS Host Port
    struct connection* clientCon = NULL;

    //The Array of Request Threads
    vector<pthread_t*> MyThreads;

    //Contains Arguments used when a thread is started
    struct iarguments* ChildArgs;

    //The Request Buffer and Length
    char Buffer[bufsize];
    int Length;

    while (true) {
        //Ideally Here we would scan over our existing (Exited) Threads and free the memory to avoid a memory leak
        //Will be implemented when I figure it out

        //Start a UDP Socket
        printf("\n\n******* [REQUEST] *******\n");
        clientCon = newServer((char*)HostAddress,53);

        //Receive a packet
        clearBuf(Buffer,bufsize);
        Length = getdat(Buffer,bufsize,clientCon);

        //Construct the arguments
        ChildArgs = (struct iarguments*)malloc(sizeof(struct iarguments)); //**Warning, this isn't ever freed
        bcpy(0,bufsize,Buffer,ChildArgs->Buffer,0);
        ChildArgs->ClientCon = clientCon;
        ChildArgs->Length = Length;

        //Create the Thread
        MyThreads.push_back( (pthread_t*)malloc(sizeof(pthread_t)) ); //**Warning, this isn't ever freed
        pthread_create(MyThreads[MyThreads.size()-1], NULL, &handlerequest, ChildArgs);

        
    }


}