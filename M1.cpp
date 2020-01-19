//Define port
#define PORT 8000
//Define Meta-Server
#define SERVER0 "10.176.69.38"//IP ADDRESS for dc07
//Define Servers 
#define SERVER1 "10.176.69.39"//IP ADDRESS for dc08
#define SERVER2 "10.176.69.40"//IP ADDRESS for dc09
#define SERVER3 "10.176.69.41"//IP ADDRESS for dc10
#define SERVER4 "10.176.69.42"//IP ADDRESS for dc11
#define SERVER5 "10.176.69.43"//IP ADDRESS for dc12
//Define Clients
#define CLIENT1 "10.176.69.36"//IP ADDRESS for dc05
#define CLIENT2 "10.176.69.37"//IP ADDRESS for dc06
//Required Librarires
#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> //closing socket
#include <errno.h>
#include <string.h>
#include <sys/types.h>// all these for socket functions and variables
#include <sys/times.h>
#include <sys/time.h>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <netdb.h>
#include <math.h>
#include <time.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <list>
#include <iterator>
#include <vector>
#include <thread>
#include <dirent.h>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <regex>
#include <queue>
#include <limits>
#include <map>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sys/time.h>
#include <algorithm>    // std::shuffle
#include <array>        // std::array
#include <random>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <iomanip>
//Global Variables
using namespace std;
#define TRUE             1
#define FALSE            0
typedef int64_t msec_t;
#define ChunkSize 4096;
msec_t MetaData[40][20][6]={0};

int fileIDCount=0;
//fileID, chunk number, server
//Global Mutex
pthread_mutex_t list_lock;
pthread_mutex_t LOCKED_lock;
pthread_mutex_t message_received_count_lock;
pthread_mutex_t message_sent_count_lock;
pthread_mutex_t exit_number_lock;
//Required functions

msec_t time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (msec_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

//msec_t now=time_ms();
string getIPAddress() //to obtain the IP address of current server
{
    string ipAddress="IP adrress not obtained";
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    success = getifaddrs(&interfaces);
    if (success == 0)
    {temp_addr = interfaces;
        while(temp_addr != NULL)
        {if(temp_addr->ifa_addr->sa_family == AF_INET)
        {if(strcmp(temp_addr->ifa_name, "en0"))
        {ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
        }
        }
            temp_addr = temp_addr->ifa_next;
        }
    }
    freeifaddrs(interfaces);
    return ipAddress;
}

class Socket_conn //The socket wrapper class *******************
{
public:
    int socket_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int iMode=0;
    Socket_conn() //Constructor to define default operations
    {
        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        bzero((char *) &address, sizeof(address));
        ioctl(socket_fd, FIONBIO, &iMode);
        address.sin_port = htons( PORT ); //converts to network byte
        address.sin_family = AF_INET; //default family
    }
    Socket_conn(const Socket_conn &sock) //Copy Constructor
    {   socket_fd=sock.socket_fd;
        new_socket=sock.new_socket;
        valread=sock.valread;
        address=sock.address;
        opt=1;
        addrlen =sock.addrlen;
        iMode=0;
    }


    
    int connect_socket(char* IPname) //Connect to the server
    {
        if(inet_pton(AF_INET, IPname, &address.sin_addr)<=0)
        {
            cout<<"Invalid address/ Address not supported "<<endl;
            return 0;
        }
        if(connect(socket_fd, (struct sockaddr *)&address, sizeof(address))<0)
        { //one of the stages of socket programming
            cout<<"Connection Failed "<<endl;
            return 0;
        }
        else
        {
            return 1;
        }
        
    }
    int listen_socket()// Listening the socket
    {
        if (setsockopt(socket_fd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),&opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_addr.s_addr = INADDR_ANY;
        if (::bind(socket_fd, (struct sockaddr *)&address, sizeof(address))<0)
        {
            perror("bind failed");
            return 0;
        }
        
        if (listen(socket_fd, 32) < 0)
        {
            perror("listen failed");
            return 0;
        }
        
    }
    
    int return_accept_response() //accepting from the socket
    {
        cout<<"waiting to connect here"<<endl;
        if ((new_socket = accept(socket_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
        {
            perror("accept failed");
            return 0;
        }
        else
        {
            return 1;
        }
        
    }
    int return_accept_socket()//return the accepted socket
    {
        return new_socket;
    }
    
    
};
class Servers //Class for servers
{
public:
    string sender;
    string receiver;
    Socket_conn socket_id;
    int connect_num;
    Servers()
    {
        sender="";
        receiver="";
        
    }
    Servers(const Servers &wrap)//copy constructor
    {   sender=wrap.sender;
        receiver=wrap.receiver;
        socket_id=wrap.socket_id;
        connect_num=wrap.connect_num;
    }
    ~Servers()//destructor
    {
    }
    
};
class fileS
{public:
    char* filename;
    int fileID;
    //vector <string> chunks;
    msec_t time;
    int chunkNos=0;
    vector <int> chunks;
    //server chunk

};
vector <fileS> FileList;

int createFile(char *data, vector <fileS> *FileList){
 fileS f;
 f.filename=data;
 fileIDCount++;
 f.fileID=fileIDCount;
 FileList->push_back(f);
 return f.fileID;
}
class Clients //Class for clients
{
public:
    string sender;
    string receiver;
    Socket_conn socket_id;
    int connect_num;
    Clients()
    {
        sender="";
        receiver="";
        
    }
    Clients(const Clients &wrap)//copy constructor
    {   sender=wrap.sender;
        receiver=wrap.receiver;
        socket_id=wrap.socket_id;
        connect_num=wrap.connect_num;
    }
    ~Clients()//destructor
    {
    }
    
};

int findServerNum(string IP)
{
    char* serverIPList[6]={SERVER0,SERVER1,SERVER2,SERVER3,SERVER4,SERVER5};
    int N=6; //Total number of Servers
    int server_num=0;
    for(int i=0;i<N;i++)// find the client number
    {
        if(IP==string(serverIPList[i]))
        {
            server_num=i;
            break;
        }
    }
    return server_num;
}

int findClientNum(string IP)
{
    char* clientIPList[2]={CLIENT1,CLIENT2};
    
    int N=2; //Total number of Clients
    int client_num=0;
    for(int i=0;i<N;i++)// find the client number
    {
        if(IP==string(clientIPList[i]))
        {
            client_num=i;
            break;
        }
    }
    return client_num + 1;
}

int findFileID(char* data){
    int mo;
    vector <fileS> :: iterator it;
    for(mo=0,it = FileList.begin(); it != FileList.end(); ++it,mo++){
        if(strcmp(data,(*it).filename)==0){
            return (*it).fileID;
        }
    }
}

int calculateChunk(long int data)
 {
    int chunkloc=data/ChunkSize;
    return chunkloc+1;
 }

 int calculateOffset(long int data)
 {
    int seekloc=data%ChunkSize;
    return seekloc;
 }

 int calculateMaxChunk(char *data){
      int mo,go,maxgo=0;
    vector <fileS> :: iterator it;
    for(mo=0,it = FileList.begin(); it != FileList.end(); ++it,mo++){
        if(strcmp(data,(*it).filename)==0){
            go= (*it).fileID;
        }
    }
    for(int a=1; a<20; a++){
        bool test=false;
        for(int b=1; b<6; b++){
            if(MetaData[go][a][b]!=0)
            {   
                test=true;
            }
        }
        if (test==true){
            maxgo++;
        }
    }
    return maxgo;
 }

int* findServers(char* data1, int chunk1){
    int IDcopy;
    int counter=0;
    IDcopy=findFileID(data1);
    int* intarr = new int[3];
    intarr[0]=0;
    intarr[1]=0;
    intarr[2]=0;
    //for(int q=0; q<10; q++){
        for(int r=0; r<6;r++){
            if(MetaData[IDcopy][chunk1][r]!=0){
                intarr[counter]=r;
                counter++;
            }
        }
        return intarr;
    //}
}

int* findRandomServer(){
   
    array<int, 5> s{ 1, 2, 3, 4, 5 };  
    unsigned seed = time_ms(); 
    shuffle(s.begin(), s.end(), default_random_engine(seed)); 
    int* intarr = new int[3];
    intarr[0]=s[0];
    intarr[1]=s[1];
    intarr[2]=s[2];

    return intarr;
}
//return array of servrers where m_sect is non zero 

long int findSize(char file_name[]) 
{ 
    // opening the file in read mode 
    FILE* fp = fopen(file_name, "r"); 
  
    // checking if the file exist or not 
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    } 
  
    fseek(fp, 0L, SEEK_END); 
  
    // calculating the size of the file 
    long int res = ftell(fp); 
  
    // closing the file 
    fclose(fp); 
  
    return res; 
} 
//char file_name[] = { "a.txt" }; 
//long int res = findSize(file_name); 
//if (res != -1) 
//printf("Size of the file is %ld bytes \n", res); 


int makeConnection(list <Servers> *SocketConnectionListServer, list <Clients> *SocketConnectionListClient)
{
    cout<<"Making Connections"<<endl;
    char* serverIPList[5]={SERVER1,SERVER2,SERVER3,SERVER4,SERVER5};
    char* clientIPList[2]={CLIENT1,CLIENT2};
    int connection_start=0,status,client_num,flag=0,rc, connection_start_1=0;
    char *setup_msg = "received";
    char *close_msg = "close";
    //own IP address open a connection
    cout<<"My Ip address is::"<<getIPAddress()<<endl;
    int valread=0;
    int server_num;
    server_num=findServerNum(getIPAddress());
    Socket_conn s1;
    //connecting to all the clients first
    s1.listen_socket();

    int counter1=0;//keeps client count
    int counter2=0;//keeps server count
    while (!(connection_start ==1)) //sending clients start mssg
    {
        int stat = s1.return_accept_response();
        if (stat ==1){
         send(s1.return_accept_socket(), setup_msg, strlen(setup_msg), 0);
        Clients w1;
        w1.sender = inet_ntoa(s1.address.sin_addr);
        w1.receiver = getIPAddress();
        w1.socket_id = s1;
        w1.connect_num = findServerNum(string(inet_ntoa(s1.address.sin_addr)));
        cout << "Connection made from "<< inet_ntoa(s1.address.sin_addr) << " to " << getIPAddress() << endl;
        counter1 = counter1 + 1;
        if(counter1 == 2)//ensuring 2 clients
        {
            connection_start=1;
        }   
        }
    }
    sleep(3);

    
    for(int i=0;i<2;i++) //receiving from clients
    {
        
        Socket_conn s1; 
        Clients w1;
        w1.sender=getIPAddress();
        w1.receiver=clientIPList[i];
        w1.connect_num=findClientNum(string(clientIPList[i]));
        int stat=s1.connect_socket(clientIPList[i]);
        w1.socket_id=s1;
        if (stat==1)
        {
        char buf[1024]={0};
        valread = read(s1.socket_fd  , buf, 1024);
        if(valread && (strcmp(buf, "received") == 0))
        {
        pthread_mutex_lock(&list_lock);
        SocketConnectionListClient->push_back(w1); //need mutex here
        pthread_mutex_unlock(&list_lock);
        cout<<"Connected "<< w1.sender <<" to "<< w1.receiver <<endl;
        }
        }
        else
        {
        cout<<"error in sending the client connect.."<<endl;
        
        }
        
    }


sleep(1); //less sleep for start 
cout<<"Connecting to the servers"<<endl;
while (!(connection_start_1 ==1)) //sending to 5 servers
    {
        int stat = s1.return_accept_response();
        if (stat ==1){
         send(s1.return_accept_socket(), setup_msg, strlen(setup_msg), 0);
        Servers w1;
        w1.sender = inet_ntoa(s1.address.sin_addr);
        w1.receiver = getIPAddress();
        w1.socket_id = s1;
        w1.connect_num = findServerNum(string(inet_ntoa(s1.address.sin_addr)));
        cout << "Connection made from "<< inet_ntoa(s1.address.sin_addr) << " to " << getIPAddress() << endl;
        counter2 = counter2 + 1;
        if(counter2 == 5)//ensuring 5 servers
        {
            connection_start_1=1;
        }   
        }
    }
    sleep(3); 
    
       for(int i=0;i<5;i++) //receiving from servers
        {
        
        Socket_conn s1; 
        Servers w1;
        w1.sender=getIPAddress();
        w1.receiver=serverIPList[i];
        w1.connect_num=findServerNum(string(serverIPList[i]));
        int stat=s1.connect_socket(serverIPList[i]);
        w1.socket_id=s1;
        if (stat==1)
        {
        char buf[1024]={0};
        valread = read(s1.socket_fd  , buf, 1024);
        if(valread && (strcmp(buf, "received") == 0))
        {
        pthread_mutex_lock(&list_lock);
        SocketConnectionListServer->push_back(w1); //need mutex here
        pthread_mutex_unlock(&list_lock);
        cout<<"Connected "<< w1.sender <<" to "<< w1.receiver <<endl;
        }
        }
        else
        {
        cout<<"error in sending the server connect.."<<endl;
        
        }
        
        }   
    
        cout<<"Connection Completed"<<endl;
        return 1;
}
/*
void calculateOffset(long int data, vector <fileS> *FileList)
 {  vector <fileS> :: iterator itt;
    vector <int> :: iterator ittt;
    int chunkloc=data/ChunkSize;
    int seekloc=data%ChunkSize;
    cout<<"Chunk number is "<<chunkloc<< " and offset loc is "<<seekloc<<endl;
    for(m=0,itt = FileList->begin(); itt != FileList->end(); ++itt,m++)
    {
        if("file1"==(*itt).filename)
        { 
            for(x=0,ittt = (*ittt).chunks->begin(); itt != (*ittt).chunks->end(); ++ittt,x++)
             {      
                 if(chunkloc==ittt)


                }
        }
    }
 }
*/
struct Reply_thread_data
{
    int socket;
    int client_num;
};


void *REPLY_CS(void *threadarg){
    struct Reply_thread_data *data;
    data = (struct Reply_thread_data *) threadarg;
    char finish_msg[100] ={0};
    char directory[200]={0};
    char filesList[100]={0};
    char* dir="/home/013/j/jx/jxk180016/aos3/";
    while(1)
    {
        char buf[1000]={0};
        int valread = read(data->socket, buf, 1000);
        if(valread!=-1){cout<<"Reading a string"<<valread<<endl;}
        //cout<<valread<<endl;
        string buffer(buf);
        size_t found0 = buffer.find("ADD");
        size_t found1 = buffer.find("WRITE");
        size_t found2 = buffer.find("READ");
        std::string delimiter = ":";
        if(valread && (found0 != string::npos))
        {
            string req;
            char* filename;
            int offset;
            size_t pos = 0;
            std::string token;
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            req=token;
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            char *token2 = new char[token.length() + 1];
            strcpy(token2, token.c_str());
            filename=token2;
            
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            offset=atoi(token.c_str());
            
            cout<<req<<" request "<<filename<<" filename "<<offset<<" offset "<<endl;

            //snprintf( directory, sizeof(directory), "%s%d", dir,server);
            //string ServerFiles=read_directory(directory);
            //snprintf( filesList, sizeof(filesList), "%s", ServerFiles.c_str());
            //send(data->socket, filesList , strlen(filesList) , 0 );
            
        }
        else if(valread && (found1 != string::npos)) //write
        {   cout<<"Inside Write"<<endl;
            string writeString,server, req;
            char* filename;
            int offset;
            size_t pos = 0;
            char response[100]={0};
            std::string token;
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            req=token;
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            char *token2 = new char[token.length() + 1];
            strcpy(token2, token.c_str());
            filename=token2;
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            writeString=atoi(token.c_str());
            
           // cout<<req<<" request "<<filename<<" filename "<<offset<<" offset "<<writeString<< " writeString"<< endl;
            
        //cout<<"crash checkpint 13"<<endl;
        
            //if filename not in list 
            
            int mo1;
            
            bool filepresent=false;
            vector <fileS> :: iterator it01;
            for(mo1=0,it01 = FileList.begin(); it01 != FileList.end(); ++it01,mo1++){
            if(strcmp(filename,(*it01).filename)==0){
            filepresent=true;
            }}
            //t
            if(filepresent==true){
            int* outarr = findServers(filename,calculateMaxChunk(filename));
            snprintf( response, sizeof(response), "%s:%d:%d:%d:%d", "RESPONSE",outarr[0],outarr[1],outarr[2],calculateMaxChunk(filename));
            send(data->socket, &response, strlen(response),0);
            }
            else if(filepresent==false){
            int* outarr = findRandomServer();    
            snprintf( response, sizeof(response), "%s:%d:%d:%d:%d", "RESPONSE",outarr[0],outarr[1],outarr[2],1);
            send(data->socket, &response, strlen(response),0);
            }
            //chunk max
            //offset by size
            //char temp1[200]={0};

        //cout<<"crash checkpint 14"<<endl;
        
            
            cout<<"response send"<<endl;

         
        //cout<<"crash checkpint 15"<<endl;   
            //char temp1[200]={0};
            //snprintf( temp1, sizeof(temp1), "%s%s/%s", dir,server.c_str(), filename.c_str());
            //cout<<temp1<<endl;
            //writeFile(temp1,writeString.c_str());
            //cout<<"written to file :"<<filename<<endl;
            //snprintf( finish_msg, sizeof(finish_msg), "%s%s", "written succesfully on file ",filename.c_str());
            //send(data->socket, &finish_msg , strlen(finish_msg) , 0 );
        }

        else if(valread && (found2 != string::npos)) //read
        {
            
            string req;
            char* filename;
            int offset, buffersize;
            size_t pos = 0;
            std::string token;
            char response[100]={0};

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            req=token;

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            char *token2 = new char[token.length() + 1];
            strcpy(token2, token.c_str());
            filename=token2;
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            offset=atol(token.c_str());
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            buffersize=atoi(token.c_str());
            
            cout<<req<<" request "<<filename<<" filename "<<offset<<" offset "<<buffersize<< " buffersize"<< endl;
            
            int* outarr = findServers(filename,calculateChunk(offset));
            //char temp1[200]={0};
            snprintf(response, sizeof(response), "%s:%d:%d:%d:%d:%d", "RESPONSE",outarr[0],outarr[1],outarr[2],calculateChunk(offset),calculateOffset(offset));
            send(data->socket, &response, strlen(response),0);
            cout<<"response send"<<endl;
            //string lastLine=getLastLine(temp1);
            //cout<<"Client "<<client<<" is reading the last data from "<<filename<<" : "<<lastLine<<endl;
            //if (lastLine.size()<3)
            //{
            //    lastLine="no content";
            //}
            //snprintf( finish_msg, sizeof(finish_msg), "%s", lastLine.c_str());
            //send(data->socket, &finish_msg , strlen(finish_msg) , 0 );
        }


}

cout<<"While of reply client ended"<<endl;
}



/*
int ReplyClient(list <Clients> *SocketConnectionListClient)
{
    pthread_t REPLY[2];
    struct Reply_thread_data Rep[2];
    int l;
    list <Clients> :: iterator itt;
    for(l=0, itt = SocketConnectionListClient->begin(); itt != SocketConnectionListClient->end(); ++itt,l++)
    {   
        Rep[l].socket=((*itt).socket_id).socket_fd;
        Rep[l].client_num=(*itt).connect_num;

        cout<<"Client Reply thread "<<(*itt).sender<<" "<<(*itt).receiver<<endl;
        int rc = pthread_create(&REPLY[l], NULL, REPLY_CS, (void *)&Rep[l]);
        if (rc)
        {
            cout<<"Problem with the creating Reply Thread.."<<endl;
            return 0;
        }
    }
    cout<<"End of Reply Client Thread"<<endl;
   // usleep(500000000);
   //sleep(1);
   //while(1){}
   
    
}
*/


struct Reply_thread_data_server
{
    int socket;
    int server_num;
};

void *REPLY_S1(void *threadarg){
    struct Reply_thread_data *data1;
    data1 = (struct Reply_thread_data *) threadarg;
    char finish_msg[100] ={0};
    char directory[200]={0};
    char filesList[100]={0};
    char* dir="/home/013/j/jx/jxk180016/aos3/";
    while(1)
    {
        char buf[200]={0};
        int valread = read(data1->socket, buf, 200);
        if(valread!=-1){cout<<"Reading a string from server "<<buf<<endl;}
        //cout<<valread<<endl;
        string buffer(buf);
        usleep(2000);
        size_t found0 = buffer.find("UPDATE");
        std::string delimiter = ":";
        if(valread && (found0 != string::npos))
        {
            string req, type; 
            int server_no, chunk ;
            msec_t updateTime;
            int offset;
            char* filename,token1;

            size_t pos = 0;
            std::string token;

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            type=token;

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            server_no=atoi(token.c_str());
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            char *token2 = new char[token.length() + 1];
            strcpy(token2, token.c_str());
            filename=token2;
            
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            chunk=atoi(token.c_str());

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            updateTime=atoll(token.c_str());

            
            cout<<server_no<<" server number "<<filename<<" filename "<<chunk<<" chunk "<<updateTime<<" time "<<endl;
            int ID_pointer;
            vector <fileS> :: iterator itt;
            int m;
            bool present=true;
            //cout<<filename<<" filename from user "<<(*itt).filename<<" filename from list"<<endl;

            for(m=0,itt = FileList.begin(); itt != FileList.end(); ++itt,m++)
            {   //cout<<typeid(filename).name()<<" filename from user "<<typeid((*itt).filename).name()<<" filename from list"<<endl;
                
                
                if(strcmp(filename,(*itt).filename)==0){
                    MetaData[((*itt).fileID)][chunk][server_no]=updateTime;
                    cout<<"writing at"<<((*itt).fileID)<<" "<<chunk<<" "<<server_no<<endl;
                    present=false;
                    break;
                }
                else{
                    present=true  ;  
                }

                }
           if(present==true){ 
           MetaData[createFile(filename,&FileList)][chunk][server_no]=updateTime;
            
           present=false;
           }
           //cout<<findFileID(filename)<< "is fileID"<<endl;
           //cout<<MetaData[0][6][1]<<"test site"<<endl;
           //cout<<FileList.size()<<" File Size"<<endl;
           int* outarr = findServers(filename,chunk);
           cout<<"The servers are "<<outarr[0]<<" "<<outarr[1]<<" "<<outarr[2]<<endl;
            
            //snprintf( directory, sizeof(directory), "%s%d", dir,server);
            //string ServerFiles=read_directory(directory);
            //snprintf( filesList, sizeof(filesList), "%s", ServerFiles.c_str());
            //send(data->socket, filesList , strlen(filesList) , 0 );
            
        }
        
}//
cout<<"while of reply server ended"<<endl;
}

int ReplyServer(list <Clients> *SocketConnectionListClient,list <Servers> *SocketConnectionListServer)
{   
    pthread_t REPLY[2];
    struct Reply_thread_data Rep[2];
    int l;
    list <Clients> :: iterator itt;
    for(l=0, itt = SocketConnectionListClient->begin(); itt != SocketConnectionListClient->end(); ++itt,l++)
    {   
        Rep[l].socket=((*itt).socket_id).socket_fd;
        Rep[l].client_num=(*itt).connect_num;

        cout<<"Client Reply thread "<<(*itt).sender<<" "<<(*itt).receiver<<endl;
        int rc = pthread_create(&REPLY[l], NULL, REPLY_CS, (void *)&Rep[l]);
        if (rc)
        {
            cout<<"Problem with the creating Reply Thread.."<<endl;
            return 0;
        }
    }
    cout<<"End of Reply Client Thread"<<endl;

    pthread_t RESPONSE[5];
    struct Reply_thread_data_server Respond[5];
    int l2;
    list <Servers> :: iterator ittt;
    for(l2=0, ittt = SocketConnectionListServer->begin(); ittt != SocketConnectionListServer->end(); ++ittt,l2++)
    {   
        Respond[l2].socket=((*ittt).socket_id).socket_fd;
        Respond[l2].server_num=(*ittt).connect_num;

        cout<<"server Reply thread "<<(*ittt).sender<<" "<<(*ittt).receiver<<endl;
        int rcx = pthread_create(&RESPONSE[l2], NULL, REPLY_S1, (void *)&Respond[l2]);
        if (rcx)
        {
            cout<<"Problem with the creating Server Reply Thread.."<<endl;
            return 0;
        }
    }
    cout<<"End of Reply Server Thread"<<endl;
   // usleep(500000000);
   while(1){}
    
}




int main(void)
{   
    //vector <fileS> FileList;
    list <Servers> SocketConnectionListServer;
    list <Clients> SocketConnectionListClient;
    int status=makeConnection(&SocketConnectionListServer,&SocketConnectionListClient);
    
    /*int start=ReplyClient(&SocketConnectionListClient);
    if(start==0)
	{
		cout<<"problem with client thread"<<endl;
		return 1;	
	}
    cout<<"Reply Server Ready"<<endl;
    */
    int start1=ReplyServer(&SocketConnectionListClient,&SocketConnectionListServer);
    //int start1=1;
    if(start1==0)
	{
		cout<<"problem with server thread"<<endl;
		return 1;	
	}
    cout<<"going to end"<<endl;
     while (1) {
        //to prevent the server from quiting and keeping it alive
    }
}

