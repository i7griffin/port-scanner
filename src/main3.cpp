// Port Scanner with Thread Pool (Phase 3)
// Scans multiple ports in parallel using worker threads

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <vector>
using namespace std;

#ifdef _WIN32
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
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
using socket_t = int;
const socket_t INVALID_SOCK = -1;
#endif

// Declared outside all the functions to be accessible for all . This is helpful for threading
std::queue<int> portQueue;
std::map<int, std::string> results;
std::mutex queueMutex;
std::mutex resultsMutex;

bool initSockets()
{
#ifdef _WIN32
    WSADATA wsaData;
    // Windows-specific struct that Winsock fills with information about itself (version, capabilities, etc.).
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    // WSAStartup(MAKEWORD(2, 2), &wsaData) — Initialize Winsock version 2.2
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

void setNonBlocking(socket_t sock)
{
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

bool waitForConnection(socket_t sock, int timeoutSeconds)
{
    fd_set writeSet, errorSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(sock, &writeSet);
    FD_SET(sock, &errorSet);

    struct timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;

    int result = select(sock + 1, nullptr, &writeSet, &errorSet, &timeout);
    return result > 0;
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
    return error == 0;
}

std::string scanSinglePort(std::string ip, int port)
{
    // TODO: Scan a single port
    // Create socket → set non-blocking → connect → waitForConnection → checkConnectionResult
    // Return "OPEN", "CLOSED", or "FILTERED"

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
    {
        return "ERROR";
    }

    socket_t sock = createsocket();
    if (sock == INVALID_SOCK)
    {
        return "ERROR";
    }

    setNonBlocking(sock);

    int result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    if (waitForConnection(sock, 2))
    {
        if (checkConnectionResult(sock))
        {
            closeSocket(sock);
            return "OPEN";
        }
        else
        {
            closeSocket(sock);
            return "CLOSED";
        }
    }
    else
    {
        closeSocket(sock);
        return "FILTERED";
    }
}

void workerThread(std::string ip)
{
    // TODO: Worker thread function
    // Loop:
    //   - Lock queueMutex
    //   - Check if portQueue is empty
    //   - If empty, break
    //   - Pop a port from portQueue
    //   - Unlock queueMutex
    //   - Scan the port using scanSinglePort()
    //   - Lock resultsMutex
    //   - Store result in results map
    //   - Unlock resultsMutex
    //   - Continue loop

    while (true)
    {
        int port;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (portQueue.empty())
            {
                break;
            }
            port = portQueue.front();
            portQueue.pop();
        }

        std::string status = scanSinglePort(ip, port);

        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results[port] = status;
        }
    }
}

int main()
{
    if (!initSockets())
    {
        cerr << "Failed to initialize sockets" << endl;
        return 1;
    }

    string ip;
    cout << "Enter IP address: " << endl;
    cin >> ip;

    int startPort, endPort;
    cout << "Enter start port: " << endl;
    cin >> startPort;
    cout << "Enter end port: " << endl;
    cin >> endPort;
    int NUM_THREADS;
    cout << "Enter number of threads (recommended 50): " << endl;
    cin >> NUM_THREADS;

    if (NUM_THREADS < 1)
    {
        cerr << "Number of threads must be at least 1" << endl;
        cleanupSockets();
        return 1;
    }

    // now we are creating a queue with the list of all the ports we want to check for .
    for (int port = startPort; port <= endPort; port++)
    {
        portQueue.push(port);
    }

    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        threads.push_back(std::thread(workerThread, ip));
        //this thread object has no name. It's an anonymous object (a temporary object).
        //this saves a lot of space 
        //this is the syntax for calling a constructor with functions and parameters without any name 
    }

    // TODO: Wait for all threads to finish
    for (auto &thread : threads)
    {
        thread.join();
    }

    // TODO: Print results
    cout << "\n=== Scan Results ===" << endl;
    for (auto &[port, status] : results)
    {
        cout << "Port " << port << " is " << status << endl;
    }

    cleanupSockets();
    return 0;
}
