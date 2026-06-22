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
const socket_t INVALID_SOCK = -1;
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
    // 1.Create SOCKET
    socket_t socket_fd = createsocket();

    // 2.Getting ip address from user
    string ip;
    cout << "Enter IP address" << endl;
    cin >> ip;

    // 3.Getting the port from the user
    int port;
    cout << "Enter port number: " << endl;
    cin >> port;

    // 4.now we are wrapping the ip address into a whole socket
    struct sockaddr_in addr;
    // memset stores the gives places with zeroes here can also do = {}
    memset(&addr, 0, sizeof(addr));

    // 5.Filling the values of the struct
    addr.sin_family = AF_INET;
    // htons help to convert the port number in the order the computer expects
    addr.sin_port = htons(port);
    // 6. converts human readable ip address strings to their packed , binary network format and wirtes it in the address field of the structure
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
    {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        closeSocket(socket_fd);
        return 1;
    }

    // 7.the next step is tho request connection to the remote server throught he particular socket . This is donw through the connect function
    int result = connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr));
    // the parameters are the file descriptor , and the socket to proceed with request and details of how big the address struct is . It results in three kinds of output dependind on the nature if the response
    // 0 == if the server accepts and the TCP handshake is successfully completed
    //  -1 == if "connection refused" error, or nothing comes back at all (filtered port, host unreachable) and it eventually times out [RST PACKET]

    if (result == 0)
    {
        // handshake completed — something is actively listening on that port
        cout << "Port " << port << " is OPEN" << endl;
    }
    else
    {
        // connection failed — port is closed or filtered
        cout << "Port " << port << " is CLOSED — " << getSocketError() << endl;
    }
}
