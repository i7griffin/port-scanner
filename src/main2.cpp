// A program which scans a particular ip address and determines whether the ports are open or closed
// Phase 2: Port range + timeout handling using non-blocking sockets and select()

#include <iostream>
#include <string>
using namespace std;

#ifdef _WIN32 // to enable functioning of program in windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
using socket_t = SOCKET;
const socket_t INVALID_SOCK = INVALID_SOCKET;
#else
// Linux and macOS headers
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>   // For non-blocking socket setup on Linux

using socket_t = int;
const socket_t INVALID_SOCK = -1;
#endif

bool initSockets()
{
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void cleanupSockets()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

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

// ===== PHASE 2: NEW FUNCTIONS =====

void setNonBlocking(socket_t sock)
{
    // TODO: Set socket to non-blocking mode
    // Windows: use ioctlsocket(sock, FIONBIO, &mode) where mode = 1
    // Linux: use fcntl(sock, F_SETFL, flags | O_NONBLOCK)
}

bool waitForConnection(socket_t sock, int timeoutSeconds)
{
    // TODO: Use select() to wait for the connection to complete with a timeout
    // Returns true if socket is ready (check result with checkConnectionResult())
    // Returns false if timeout occurred
    
    // Step 1: Create fd_set for write and error conditions
    // Step 2: Call select() with timeout
    // Step 3: Return true if result > 0, false if timeout/error
    
    return false;  // placeholder
}

bool checkConnectionResult(socket_t sock)
{
    // TODO: Call getsockopt(SO_ERROR) to check if connection succeeded
    // Returns true if error == 0 (port is OPEN)
    // Returns false if error != 0 (port is CLOSED)
    
    return false;  // placeholder
}

// ===== END PHASE 2 FUNCTIONS =====

int main()
{
    if (!initSockets())
    {
        cerr << "Failed to initialize sockets" << endl;
        return 1;
    }

    // 1. Get IP address from user
    string ip;
    cout << "Enter IP address: " << endl;
    cin >> ip;

    // 2. Get port range from user
    int startPort, endPort;
    cout << "Enter start port: " << endl;
    cin >> startPort;
    cout << "Enter end port: " << endl;
    cin >> endPort;

    // 3. Set up address struct (once, then update port in loop)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    // 4. Convert IP address
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
    {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        cleanupSockets();
        return 1;
    }

    // 5. Scan the port range
    for (int port = startPort; port <= endPort; port++)
    {
        // TODO: Create socket
        // TODO: Check if socket creation succeeded
        
        // TODO: Set socket to non-blocking mode
        
        // TODO: Update addr.sin_port for this port (use htons)
        
        // TODO: Call connect() (expect it to return -1 with EINPROGRESS on non-blocking)
        
        // TODO: Call waitForConnection() with timeout (e.g., 2 seconds)
        // TODO: If waitForConnection() returns false (timeout), print "FILTERED" and continue
        
        // TODO: If waitForConnection() returns true, call checkConnectionResult()
        // TODO: If checkConnectionResult() returns true, print "Port X is OPEN"
        // TODO: If checkConnectionResult() returns false, print "Port X is CLOSED"
        
        // TODO: Close socket
    }

    cleanupSockets();
    return 0;
}
