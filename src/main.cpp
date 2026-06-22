// A program which scans a particular ip address and determines whether the ports are open or closed
#include <iostream>
#include <string>
using namespace std;
#ifdef _WIN32 // to enable functioning of program in windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // prevents windows.h from importing many other useless api UIs and other stuff and from also importing older sockets
#endif
#include <windows.h>
#include <winsock2.h> // contains socket() , Connection handling: bind(), listen(), accept(), connect() , ata transfer: send(), recv()
#include <ws2tcpip.h> // core windows socket file , getaddrinfo , inet_pton , struct addrinfo

#pragma comment(lib, "Ws2_32.lib")
using socket_t = SOCKET;
const socket_t INVALID_SOCK = INVALID_SOCKET;
#else
// Linux and macOS headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using socket_t = int;
onst socket_t INVALID_SOCK = -1;
#endif


string getSocketError()
{
#ifdef _WIN32
    return std::to_string(WSAGetLastError());
#else
    return std::string(strerror(errno));
#endif
}

socket_t createsocket()
{
    socket_t socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == INVALID_SOCK)
    {
        std::cerr << "Failed to create socket: " << getSocketError() << std::endl;
    }
    return socket_fd;
}

void closeSocket(socket_t sock)
{
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

int main()
{
    //1.Create SOCKET
    socket_t socket_fd = createsocket() ;

    //2.Getting ip address from user 
    string ip ;
    cout << "Enter IP address" << endl ;
    cin >> ip ;


    struct in_addr address;
    // converts human readable ip address strings to their packed , binary network format
    if (inet_pton(AF_INET, ip.c_str(), &address) != 1)
    {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        closeSocket(socket_fd);
        return 1;
    }

    // Thia assigns a particular file descriptor to the socket created
    // AF_INET = uses ipv4 address
    // SOCK_STREAM = the socket uses tcp protocol

    struct sockaddr_in addr()

        return 0;
}
