// This is a upgraded port scanner with banner grabbing possible
// improved upon the phase 3 port scanner with banner grabbing

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


std::queue<int> portQueue;
std::map<int, std::string> results;
std::mutex queueMutex;
std::mutex resultsMutex;



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



std::string getProbeForPort(int port)
{
    
    return "";
}

std::string grabBanner(socket_t sock, int port)
{
   

    std::string probe = getProbeForPort(port);

    if (!probe.empty())
    {
        
    }

    
    if (waitForConnection(sock, 2))
    {
       
        char buffer[1024];
        
    }

    return ""; 
}

std::string identifyService(int port, std::string banner)
{
    

    if (banner.empty())
    {
        return "Unknown";
    }

    
    return "Unknown Service";
}

std::string scanSinglePort(std::string ip, int port)
{
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
            
            std::string banner = grabBanner(sock, port);
            std::string service = identifyService(port, banner);

            closeSocket(sock);

           
            return "OPEN - " + service;
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
        cout << "Port " << port << " is " << status << endl;

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

    int numThreads;
    cout << "Enter number of threads (recommended 50): " << endl;
    cin >> numThreads;

    if (numThreads < 1)
    {
        cerr << "Number of threads must be at least 1" << endl;
        cleanupSockets();
        return 1;
    }

    // creating a queue of ports requested to scan by the user
    for (int port = startPort; port <= endPort; port++)
    {
        portQueue.push(port);
    }

    // using worker thread from the main thread
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; i++)
    {
        threads.push_back(std::thread(workerThread, ip));
    }

    // main thread will wait for the worker threads to finish
    for (auto &thread : threads)
    {
        thread.join();
    }

    // Print results
    cout << "\nThis is the scan results " << endl;
    for (auto &[port, status] : results)
    {
        cout << "Port " << port << " is " << status << endl;
    }

    cleanupSockets();
    return 0;
}
