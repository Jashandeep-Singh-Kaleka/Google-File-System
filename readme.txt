Steps to compile
1. Open 5 consoles for servers and compile and run the server code by
- compiling first with g++ S1.cpp -o server  -lpthread -std=c++11
- running by ./server
2. Open 2 consoles for the clients and compile and run client code by
- compiling first with g++ C1.cpp -o client  -lpthread -std=c++11
- running by ./client
3. Open 1 console for the metaData server and compile and run meta server code by
- compiling it by g++ M1.cpp -o m-server  -lpthread -std=c++11
- running it by ./m-server
4. Clients can be instructed to make a read or a write and the reads and writes go concurrently.