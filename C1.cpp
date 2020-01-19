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
//Global Variables
using namespace std;
#define TRUE             1
#define FALSE            0
typedef int64_t msec_t;
#define ChunkSize 4096;
int message_sent_count=0;
int message_received_count=0;
pthread_t start_Quorum[30];
//Global Mutex
pthread_mutex_t list_lock;
pthread_mutex_t LOCKED_lock;
pthread_mutex_t message_received_count_lock;
pthread_mutex_t message_sent_count_lock;
pthread_mutex_t exit_number_lock;
//Required functions
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

msec_t time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (msec_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// All the stages to Establish a socket connection
class Socket_conn
{
public:
    int socket_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int iMode=0;
    Socket_conn() //default steps
    {if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
        bzero((char *) &address, sizeof(address));
        ioctl(socket_fd, FIONBIO, &iMode);
        address.sin_port = htons( PORT );
        address.sin_family = AF_INET;
        
    }
    //Constructor
    Socket_conn(const Socket_conn &sock)
    {
        socket_fd=sock.socket_fd;
        new_socket=sock.new_socket;
        valread=sock.valread;
        address=sock.address;
        opt=1;
        addrlen =sock.addrlen;
        iMode=0;
        
    }
    ~Socket_conn()
    {
        
    }
    //making connection
    int connect_socket(char* IPname)
    {
        if(inet_pton(AF_INET, IPname, &address.sin_addr)<=0)
        {
            cout<<"Invalid address/ Address not supported "<<endl;
            return 0;
        }
        
        if(connect(socket_fd, (struct sockaddr *)&address, sizeof(address))<0)
        {
            cout<<"Failed "<<endl;
            return 0;
        }
        else
        {
            return 1;
        }
        
        
        
        
    }
    int listen_socket()
    {
        //Once connected you need to bind and listen
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
    int return_accept_response()
    {
        cout<<"waiting"<<endl;
        if ((new_socket = accept(socket_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
        {
            perror("failed to accept");
            return 0;
        }
        else
        {
            return 1;
        }
        
    }
    int return_accept_socket()
    {
        return new_socket;
    }
    
    
};

class Servers 
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

class Clients 
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



int makeConnection(list <Servers> *SocketConnectionList,list <Servers> *SocketConnectionListServer)
{
    cout<<"Making Connections"<<endl;
    char* serverIPList[6]={SERVER0,SERVER1,SERVER2,SERVER3,SERVER4,SERVER5};
    char* clientIPList[2]={CLIENT1,CLIENT2};
    int connection_start=0,status,client_num,flag=0,rc, connection_start_1=0;
    char *setup_msg = "received";
    char *close_msg = "close";
    int valread=0;
    //own IP address open a connection
    cout<<"My Ip address is::"<<getIPAddress()<<endl;
    for (int i = 0; i < 6; i++) {
        Socket_conn s1;
        Servers w1; //client and server connection
        w1.sender = getIPAddress();
        w1.receiver = serverIPList[i];
        w1.connect_num = findServerNum(string(serverIPList[i]));
        
        int stat = s1.connect_socket(serverIPList[i]);
        w1.socket_id = s1;
        //cout<<w1.socket_id<<"socket_id"<<endl;
        if (stat == 1)
        {
            char buf[1024] = { 0 };
            valread = read(s1.socket_fd, buf, 1024);
            if (valread && (strcmp(buf, "received") == 0))
            {  //cout<<SocketConnectionListServer->size()<<endl;
                pthread_mutex_lock(&list_lock);
                //SocketConnectionListServer->push_back(w1);
                pthread_mutex_unlock(&list_lock);
                cout << "Connection made from " << w1.sender << " to " << w1.receiver << endl;
                //the client is connected to the server as server send back a receive message
            }
        }
        else {
            cout << "error in sending the client connection" << endl;
        }
    }
    Socket_conn s1;
    int counter2=0;
    s1.listen_socket();
    while (!(connection_start == 1)) {
        int stat = s1.return_accept_response();
        if (stat == 1) {
            //cout<<s1.return_accept_socket()<<endl;
            send(s1.return_accept_socket(), setup_msg, strlen(setup_msg), 0);
            Servers w1;
            w1.sender = inet_ntoa(s1.address.sin_addr);
            w1.receiver = getIPAddress();
            w1.socket_id = s1;
            //cout<<w1.socket_id<<"socket_id"<<endl;
            w1.connect_num = findServerNum(string(inet_ntoa(s1.address.sin_addr)));
            cout<<"connection number "<<w1.connect_num<<endl;
            if(w1.connect_num==0){
                SocketConnectionListServer->push_back(w1);
            }
            pthread_mutex_lock(&list_lock);
            SocketConnectionList->push_back(w1);
            pthread_mutex_unlock(&list_lock);
            cout << "Connected "<< inet_ntoa(s1.address.sin_addr) << " to " << getIPAddress() << endl;
            //cout<<SocketConnectionListServer->size()<<endl;
             
            if (SocketConnectionList->size() == 6)
            {
                connection_start = 1;
                
            }
            //counter2=counter2+1;
            //if(counter2==7){
             //   connection_start=1;
            //}
        }
        else
        {
            cout << "couldnt connect to socket-receiver side"<< endl;
        }
    }

   cout << "Connection Complete" << endl;
}

struct sendRequest
{
    int socket;
    string serverName;
    msec_t time; 
};
struct receiveReply
{
    int socket1;
    string serverName1;
    msec_t time1; 
};

struct receiveReplyW
{
    int socket2;
    string serverName2;
    msec_t time2; 
};

int sendRequestServer(list <Servers> *SocketConnectionListServer,list <Servers> *SocketConnectionList)
{
    list <Servers> :: iterator itt;
    struct sendRequest data;
    
    msec_t now=time_ms();
    char request[100]={0};
    char req11[6]={0};
    char filename[20]={0};
    long int offset=0;
    string bufx;
    std::string writer;
    
    int bufz=0;
    int valread,m;
    cout<<"Enter a Request"<<endl;
    cin>>req11;
    cout<<"Enter Filename"<<endl;
    cin>>filename;
    usleep(300);
    if(strcmp(req11,"Write")==0 || strcmp(req11,"WRITE")==0){
    cout<<"Enter string to Append"<<endl;
    cin.clear();
    cin.sync();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, writer);}
    if(strcmp(req11,"Read")==0 || strcmp(req11,"READ")==0){
    cout<<"Enter Offset"<<endl;
    cin>>offset;
    cout<<"Enter size of Read"<<endl;
    cin>>bufz;}
    
    
    for(m=0,itt = SocketConnectionListServer->begin(); itt != SocketConnectionListServer->end(); ++itt,m++){   
    data.serverName=(*itt).sender;
    data.socket=((*itt).socket_id).return_accept_socket();
    data.time=time_ms();
    if(strcmp(req11,"Read")==0 || strcmp(req11,"READ")==0){
    snprintf(request, sizeof(request), "%s:%s:%d:%d", req11, filename, offset, bufz);}
    if(strcmp(req11,"Write")==0 || strcmp(req11,"WRITE")==0){
    snprintf(request, sizeof(request), "%s:%s:%d", req11, filename, writer.length());}    
    cout<<"sending message "<<request<<"to"<<data.serverName<<endl;
    if(send(data.socket, &request, strlen(request), 0)){
    cout<<"sending Request Message"<<endl;}
    pthread_mutex_lock(&message_sent_count_lock);
    message_sent_count++;
    pthread_mutex_unlock(&message_sent_count_lock);
    char buf1[1024]={0};
    //sleep(1);
    int valread = read(data.socket, buf1, 1024);
    //cout<<buf1<<endl;
    if(valread!=-1){cout<<"Reading a string"<<endl;}
        //cout<<valread<<endl;
        string buffer(buf1);
        
        size_t found0 = buffer.find("RESPONSE");

        
        std::string delimiter = ":";

        
        if(valread && (found0 != string::npos))
        {
            string req;
            char* filename1;
            int server1, server2, server3, chunkno;
            long int offsetno;

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
            server1=atoi(token.c_str());

        
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            server2=atoi(token.c_str());

        
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            server3=atoi(token.c_str());

       
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            chunkno=atoi(token.c_str());

        
            if(strcmp(req11,"Read")==0 || strcmp(req11,"READ")==0){
            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            offsetno=atol(token.c_str());}

        
            //cout<<req<<"server no 1 "<<server1<<" server no 2 "<<server2<<" server no 3 "<<server3<<" chunk no "<<chunkno<<" offset "<<offsetno<<endl;;
            //if read

            if(strcmp(req11,"Read")==0 || strcmp(req11,"READ")==0){
                
        
                list <Servers> :: iterator ittx;
                struct receiveReply data1;
                char requestR[100]={0};
                int mot;
                for(mot=0,ittx = SocketConnectionList->begin(); ittx != SocketConnectionList->end(); ++ittx,mot++){
                    data1.serverName1=(*ittx).sender;
                    data1.socket1=((*ittx).socket_id).return_accept_socket();
                    data1.time1=time_ms();
                    if(findServerNum((*ittx).sender)==server1)
                    {
                        snprintf(requestR, sizeof(requestR), "%s:%s:%d:%d:%d", "READ", filename, chunkno, offsetno, bufz );
                        cout<<"sending message "<<requestR<<"to"<<data1.serverName1<<endl;
                        if(send(data1.socket1, &requestR, strlen(requestR), 0)){
                        cout<<"sending Request Message to servers"<<endl;}
                        char buf1R[4096]={0};
                        //sleep(1);
                        int valreadR = read(data1.socket1, buf1R, 4096);
                        cout<<buf1R<<endl;

                    }
            
            }}
            //ifwrite
               if(strcmp(req11,"Write")==0 || strcmp(req11,"WRITE")==0){
                
            
                list <Servers> :: iterator ittxy;
                struct receiveReplyW data2;
                char requestW[1024]={0};
                int moty;
                vector <int> servs;
                servs.push_back(server1);
                servs.push_back(server2);
                servs.push_back(server3);

                vector <int>:: iterator ixt;

            for(ixt=servs.begin(); ixt!=servs.end(); ixt++){
                for(moty=0,ittxy = SocketConnectionList->begin(); ittxy != SocketConnectionList->end(); ++ittxy,moty++)
                {
                    data2.serverName2=(*ittxy).sender;
                    data2.socket2=((*ittxy).socket_id).return_accept_socket();
                    data2.time2=time_ms();
                    //cout<<server1<<" "<<findServerNum((*ittxy).sender)<<endl;
                    //cout<<server2<<" "<<findServerNum((*ittxy).sender)<<endl;
                    //cout<<server3<<" "<<findServerNum((*ittxy).sender)<<endl;
                    
                    
                    if(*ixt==findServerNum((*ittxy).sender))
                    {   
                        cout<<"stuff to be written "<<filename<<" "<<chunkno<<" "<<writer.length()<<" "<<writer.c_str()<<endl;
                        snprintf(requestW, sizeof(requestW), "%s:%s:%d:%d:%s", "WRITE", filename, chunkno, writer.length(), writer.c_str() );
                        
                        cout<<"sending message "<<requestW<<"to"<<data2.serverName2<<endl;
                        
                        if(send(data2.socket2, &requestW, strlen(requestW), 0)){
                        cout<<"sending Request Message to servers"<<endl;}
                        char buf1W[1024]={0};
                        //sleep(1);
                        
                        int valreadW = read(data2.socket2, buf1W, 4096);
                        //add code for commit
                        cout<<buf1W<<endl;
                        usleep(3000);

                    }
            
            }
            }
            
            }
         
           
        }

    //what to do with the answers
    //if read print
    //if write 
    // will need threading
    //the reply will include server number to whom it has to send a request for a read or a write or an append 
    //append is same as quorum
    }

}
//send filename, offset

int main(void)
{
    
    list <Servers> SocketConnectionList;
    list <Servers> SocketConnectionListServer;//m-server
    
    list <Clients> SocketConnectionListClient;
    
    int status=makeConnection(&SocketConnectionList,&SocketConnectionListServer);
    cout<<"going to end"<<endl;
    cout<<SocketConnectionList.size()<<" List Size"<<endl;
    cout<<SocketConnectionListServer.size()<<"Server List Size"<<endl;
    cout<<SocketConnectionListClient.size()<<" Client List Size"<<endl;
    for(int z=0;z<30;z++)
    {int rz=sendRequestServer(&SocketConnectionListServer,&SocketConnectionList);}
     while (1) {
        //to prevent the server from quiting and keeping it alive
    }
}

    