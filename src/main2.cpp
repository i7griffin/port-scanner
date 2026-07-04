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
#include <fcntl.h> // For non-blocking socket setup on Linux

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
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0); // Gets the current file/socket flags.
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif // Adds the O_NONBLOCK flag while keeping existing flags.
    // The socket now returns immediately instead of waiting for data.
    // Windows uses ioctlsocket(sock, FIONBIO, &mode) where mode = 1 . 1 means enable non blobking and 0 means disable non blocking
    // Linux uses fcntl(sock, F_SETFL, flags | O_NONBLOCK)
}

bool waitForConnection(socket_t sock, int timeoutSeconds)
{
    fd_set writeSet, errorSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    /* These ae sets that contain file descriptors named according to our need.
    we will generally at the start add the socket to both of the sets .*/
    FD_SET(sock, &writeSet);
    FD_SET(sock, &errorSet);

    struct timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    /* Creating a time structure to ask the select function to wait for a particular time.*/

    int result = select(sock + 1, nullptr, &writeSet, &errorSet, &timeout);
    /* if it is in writeset 1 will be returned or else 0 */

    if (result > 0)
    {
        return true; // Connection finished (success or failure)
    }
    else
    {
        return false; // Timeout
    }
}

bool checkConnectionResult(socket_t sock)
{
    int error = 0;

#ifdef _WIN32
    int errorLen = sizeof(error);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&error, &errorLen);
#else
    socklen_t errorLen = sizeof(error);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)&error, &errorLen);
#endif

    return error == 0; // 0 means connection succeeded
    // amd it returns any other if the connection is failed
}

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
        socket_t sock = createsocket();
        if (sock == INVALID_SOCK)
        {
            continue;
            // since we are checking through a range of ports we will skip to the next port
        }

        setNonBlocking(sock);

        // I will update addr.sin_port
        addr.sin_port = htons(port);

        int result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        //(struct sockaddr *)&addr — pointer to the address struct (cast to generic sockaddr*)

        if (waitForConnection(sock, 2))
        {
            if (checkConnectionResult(sock))
            {
                cout << "Port " << port << " is OPEN" << endl;
            }
            else
            {
                cout << "Port " << port << " is CLOSED" << endl;
            }
            closesocket(sock);
        }
        else
        {
            // Port is filtered
            cout << "Port " << port << " is FILTERED" << endl;
        }
        closeSocket(sock);
    }

    cleanupSockets();
    return 0;
}
