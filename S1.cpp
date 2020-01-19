//g++ s2.cpp -o server2 -lpthread -std=c++11
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


int makeConnection(list <Servers> *SocketConnectionList, list <Clients> *SocketConnectionListClient, list <Servers> *SocketConnectionListServer)
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
    int conter2=0;//keep the server count

    //send clients that are connected the start message
    while (!(connection_start ==1))
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
        cout<<"attempting to look for clients"<<endl;
        if(counter1 == 2)//ensuring 2 clients
        {
            connection_start=1;
        }   
        }
    }

    sleep(3);

    //receiving from clients that sent the receive message
    for(int i=0;i<2;i++)
    {   Socket_conn s1; 
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
    sleep(2);
    cout<<"Connecting to m-server"<<endl;
    //receiving from  to m-server
    for (int i = 0; i < 1; i++) 
    {
        Socket_conn s1;
        Servers w1; //client and server connection
        w1.sender = getIPAddress();
        w1.receiver = SERVER0;
        w1.connect_num = findServerNum(string(SERVER0));
        int stat = s1.connect_socket(SERVER0);
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
            cout << "error in sending the server connection" << endl;
        }
    }

    //Socket_conn s1;
    //s1.listen_socket();
    while (!(connection_start_1 == 1)) {
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
            pthread_mutex_lock(&list_lock);
            SocketConnectionList->push_back(w1);
            pthread_mutex_unlock(&list_lock);
            cout << "Connected "<< inet_ntoa(s1.address.sin_addr) << " to " << getIPAddress() << endl;
            //cout<<SocketConnectionListServer->size()<<endl;
             
            if (SocketConnectionList->size() == 1)
            {
                connection_start_1 = 1;
                
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

    //for : to increment the server num
    //if my server num for
    //else while 
    cout<<"Connecting among servers"<<endl;
    sleep(2);
    for (int q=1; q<6; q++)
    {   int connection_start_2=0, counter3=0;
        if(findServerNum(getIPAddress())==q)
        {   sleep(1);
            for(int i=0;i<5;i++)
            {if(i==q-1)
            {   cout<<"This is server "<<q<<endl;
                continue;
            } 
            else
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
                    cout<<"error in sending the client connect.."<<endl;
                }
        
            }
            }
        }
        else
        {
            while (!(connection_start_2 ==1))
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
                counter3 = counter3 + 1;
                if(counter3 == 1)//ensuring 4 servers
                {
                    connection_start_2=1;
                }   
                }
            }
        }
    }
        cout<<SocketConnectionListServer->size()<<" Server list"<<endl;
        cout<<SocketConnectionList->size()<<" M-server list"<<endl;
        cout<<SocketConnectionListClient->size()<<" Client list"<<endl;
    
        cout<<"Connection Completed"<<endl;
        return 1;
}



struct sendHeartBeat
{
    int socket;
    string serverName;
    msec_t time; 
};

//heartbeat 
int HeartBeat(list <Servers> *SocketConnectionList){
    char files[200];
    char* name="/home/013/j/jx/jxk180016/aos3/";
    char integer_str[1];
    int qr=findServerNum(getIPAddress());
    sprintf(integer_str, "%d", qr );
    //cout<<"server number is "<<endl;
    size_t len = strlen(name);
    char* ret = new char[len+1];
    strcpy(ret, name);
    strcat(ret, integer_str);
    //cout<<ret<<" is file access path"<<endl;
    DIR* dirp = opendir(ret);
    list<string> filesList;
    list <Servers> :: iterator itt;
    struct sendHeartBeat data;
    struct dirent * dp;
    
    
    while ((dp = readdir(dirp)) != NULL)
    {
        
        if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 || string(dp->d_name).back()=='~')
            continue;
        
        filesList.push_back(dp->d_name);
        
    }
    closedir(dirp);
    filesList.sort();
    list<string>::iterator it2;
    
    
    for (it2 = filesList.begin(); it2 != filesList.end(); it2++)
    {   cout<<*it2<<endl;
        strcat(files,(*it2).c_str());
        strncat(files,",",sizeof(","));
        //cout<<*it2<<endl;
        char chunk_info[1000]={0};
        int m;
        std::string s = (*it2).c_str();
        size_t pos=0;
        std::string delimiter = ".";
        std::string token, token1;
        pos =s.find(delimiter);
        token = s.substr(0, s.find(delimiter));
        s.erase(0, pos + delimiter.length());
        pos =s.find(delimiter);
        token1 = s.substr(0, s.find(delimiter));
        s.erase(0, pos + delimiter.length());
        //cout<<token<<" and chunk "<<token1<<endl;
        snprintf(chunk_info, sizeof(chunk_info), "%s:%d:%s:%s:%lu", "UPDATE",findServerNum(getIPAddress()), token.c_str(), token1.c_str(),time_ms());
        cout<<"the data sending is "<<chunk_info<<endl;

        for(m=0,itt = SocketConnectionList->begin(); itt != SocketConnectionList->end(); ++itt,m++){   
        data.serverName=(*itt).sender;
        data.socket=((*itt).socket_id).return_accept_socket();
        data.time=time_ms();

        //cout<<data.serverName<<" is Server Name,"<<data.socket<<" is Socket Name,"<<data.time<<" is Time"<<endl;


        if(send(data.socket, &chunk_info, strlen(chunk_info), 0)){
        //cout<<"sending Request Message"<<endl;
        }
        usleep(2000);
        }
    }

    
}

//REPEATING HEARTBEAT

//heartbeat 
void HeartBeatRepeat(){
   
    cout<<"Heart Beat Repeat"<<endl;
    char files[200];
    char* name="/home/013/j/jx/jxk180016/aos3/";
    char integer_str[1];
    int qr=findServerNum(getIPAddress());
    sprintf(integer_str, "%d", qr );
    //cout<<"server number is "<<endl;
    size_t len = strlen(name);
    char* ret = new char[len+1];
    strcpy(ret, name);
    strcat(ret, integer_str);
    //cout<<ret<<" is file access path"<<endl;
    DIR* dirp = opendir(ret);
    list<string> filesList;
    list <Servers> :: iterator itt;
    struct sendHeartBeat data;
    struct dirent * dp;
    
    
    while ((dp = readdir(dirp)) != NULL)
    {
        
        if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0 || string(dp->d_name).back()=='~')
            continue;
        
        filesList.push_back(dp->d_name);
        
    }
    closedir(dirp);
    filesList.sort();
    list<string>::iterator it2;
    
    
    for (it2 = filesList.begin(); it2 != filesList.end(); it2++)
    {   cout<<*it2<<endl;
        strcat(files,(*it2).c_str());
        strncat(files,",",sizeof(","));
        //cout<<*it2<<endl;
        char chunk_info[1000]={0};
        int m;
        std::string s = (*it2).c_str();
        size_t pos=0;
        std::string delimiter = ".";
        std::string token, token1;
        pos =s.find(delimiter);
        token = s.substr(0, s.find(delimiter));
        s.erase(0, pos + delimiter.length());
        pos =s.find(delimiter);
        token1 = s.substr(0, s.find(delimiter));
        s.erase(0, pos + delimiter.length());
        //cout<<token<<" and chunk "<<token1<<endl;
        snprintf(chunk_info, sizeof(chunk_info), "%s:%d:%s:%s:%lu", "UPDATE",findServerNum(getIPAddress()), token.c_str(), token1.c_str(),time_ms());
        //cout<<"the data sending is "<<chunk_info<<endl;

        data.serverName="10.176.69.38";
        data.socket=16;
        data.time=time_ms();

        //cout<<data.serverName<<" is Server Name,"<<data.socket<<" is Socket Name,"<<data.time<<" is Time"<<endl;


        if(send(data.socket, &chunk_info, strlen(chunk_info), 0)){
        //cout<<"sending Request Message"<<endl;
        }
        usleep(2000);
        
    }

    
}

int getFileSizes(ifstream *file) {
    file->seekg(0,ios::end);
    int filesize = file->tellg();
    file->seekg(ios::beg);
    return filesize;

}
struct Reply_thread_data
{
    int socket;
    int client_num;
    int socket_list[2];
};

void *REPLYC(void *threadarg)
{
    struct Reply_thread_data *data;
    data = (struct Reply_thread_data *) threadarg;
    char finish_msg[100] ={0};
    char filesList[100]={0};
    
    while(1)
    {
        
        char buf[1066]={0};
        
        int valread = read(data->socket, buf, 100);
        
        string buffer(buf);
        
        size_t found0 = buffer.find("READ");
        size_t found1 = buffer.find("WRITE");
        size_t found2 = buffer.find("COMMIT");
        std::string delimiter = ":";
        if(valread && (found0 != string::npos))
        {
            string req;
            char* filename;
            int offset, byterange, chunkno;
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
            chunkno=atoi(token.c_str());

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            offset=atoi(token.c_str());

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            byterange=atoi(token.c_str());


            char* name="/home/013/j/jx/jxk180016/aos3/";
            char integer_str[1];
            int qr=findServerNum(getIPAddress());
            sprintf(integer_str, "%d", qr );
            size_t len = strlen(name);
            char* ret = new char[len+1];
            strcpy(ret, name);
            strcat(ret, integer_str);   

            string finalaccess;
            finalaccess.clear();
            finalaccess.append(ret);
            finalaccess.append("/");
            finalaccess.append(filename);
            finalaccess.append(".");
            finalaccess.append(to_string(chunkno));
            finalaccess.append(".txt");
            cout<<finalaccess<<" final access"<<endl;
            
            ifstream  afile;
            char *buffer100 = new char[byterange];
            afile.open(finalaccess, ios::out | ios::in );
            afile.seekg(offset ,ios::cur);
            afile.read(buffer100,byterange);

            char bufsend[5000]={0};
            snprintf(bufsend, sizeof(bufsend), "%s:%s", "The read value is", buffer100);
            send(data->socket, bufsend , strlen(bufsend) , 0 );
            afile.close();

        }
        else if(valread && (found1 != string::npos))
        {
            string req;
            char* filename;
            int offset, byterange, chunkno, lenght;
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
            chunkno=atoi(token.c_str());

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            lenght=atoi(token.c_str());

            pos = buffer.find(":");
            token = buffer.substr(0, buffer.find(":"));
            buffer.erase(0, pos + delimiter.length());
            char *token3 = new char[token.length() + 1];
            strcpy(token3, token.c_str());
            char *buffer10=new char[1066];
            buffer10=token3;


            char* name="/home/013/j/jx/jxk180016/aos3/";
            char integer_str[1];
            int qr=findServerNum(getIPAddress());
            sprintf(integer_str, "%d", qr );
            size_t len = strlen(name);
            char* ret = new char[len+1];
            strcpy(ret, name);
            strcat(ret, integer_str);   

            string finalaccess;
            finalaccess.clear();
            finalaccess.append(ret);
            finalaccess.append("/");
            finalaccess.append(filename);
            finalaccess.append(".");
            finalaccess.append(to_string(chunkno));
            finalaccess.append(".txt");
            cout<<finalaccess<<" final access"<<endl;
            
            ifstream  afile;
            afile.open(finalaccess, ios::in | ios::binary | ios::out);
            int fileSize = getFileSizes(&afile);

            if((4096-fileSize)<lenght){
                
            finalaccess.clear();
            finalaccess.append(ret);
            finalaccess.append("/");
            finalaccess.append(filename);
            finalaccess.append(".");
            finalaccess.append(to_string(chunkno+1));
            finalaccess.append(".txt");
            std::ofstream file { finalaccess };
            ofstream bfile;
            bfile.open(finalaccess, ios::in | ios::out | ios::app);
            bfile.seekp(fileSize , ios::cur);
            bfile.write(buffer10, lenght);

            char bufsend[50]={0};
            snprintf(bufsend, sizeof(bufsend), "%s:%d", "The write has happened at", chunkno);
            send(data->socket, bufsend , strlen(bufsend) , 0 );
            bfile.close();


            }
            else{
                ofstream bfile;
                bfile.open(finalaccess, ios::in | ios::out | ios::app);

                bfile.seekp(fileSize , ios::cur);
                bfile.write(buffer10, lenght);

                char bufsend[50]={0};
                snprintf(bufsend, sizeof(bufsend), "%s:%d", "The write has happened at", chunkno);
                send(data->socket, bufsend , strlen(bufsend) , 0 );
                bfile.close();
            }



        
            

        }
    }          

}

int ReplyClient(list <Clients> *SocketConnectionListClient, list <Servers> *SocketConnectionList)
{
    
    pthread_t REPLY[2];
    struct Reply_thread_data Rep[2];
    int la;
    //int socket_list[6];
    list <Clients> :: iterator itta;
    cout<<"inside reply client"<<endl;
    

    for(la=0, itta = SocketConnectionListClient->begin(); itta != SocketConnectionListClient->end(); ++itta,la++)
    {
        
        Rep[la].socket=((*itta).socket_id).socket_fd;
        Rep[la].client_num=(*itta).connect_num;
        
        int rc = pthread_create(&REPLY[la], NULL, REPLYC, (void *)&Rep[la]);
        if (rc)
        {
            cout<<"Problem with the creating Reply Thread.."<<endl;
            return 0;
        }
    }
     HeartBeatRepeat();
    while(1){
        //heartbeat
        //int status1=HeartBeat(&SocketConnectionList);
        sleep(5);
        HeartBeatRepeat();
    }

}    


int main(void)
{   //**************************
    //One thread to keep sending heartbeat messages
    //Another to perform operations asked by the Client
    //
    list <Servers> SocketConnectionListServer;
    list <Servers> SocketConnectionList;//m-server
    list <Clients> SocketConnectionListClient;
    int status=makeConnection(&SocketConnectionList,&SocketConnectionListClient,&SocketConnectionListServer);
    cout<<"going to end"<<endl;
    cout<<"This is server "<<findServerNum(getIPAddress())<<endl;
    sleep(5);
    int status1=HeartBeat(&SocketConnectionList);

    int start1=ReplyClient(&SocketConnectionListClient, &SocketConnectionList);
    //int start1=1;
    if(start1==0)
	{
		cout<<"problem with server thread"<<endl;
		return 1;	
	}
    cout<<"going to end"<<endl;
   
    //reply thread for clients
    //reply to m-server
     while (1) {
        //to prevent the server from quiting and keeping it alive
    }
}

