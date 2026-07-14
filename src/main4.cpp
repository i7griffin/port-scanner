// This is a upgraded port scanner with banner grabbing possible
// improved upon the phase 3 port scanner with banner grabbing
#include "services.h"
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

// this data declaring at the global level helps in threading
std::queue<int> portQueue;
std::map<int, std::string> results;
std::mutex queueMutex;
std::mutex resultsMutex;

// FIX: added a dedicated mutex to stop threads interleaving cout output
std::mutex printMutex;

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

// FIX: new function — watches the READ set instead of the WRITE set.
// waitForConnection() tells you when connect() has finished (socket becomes writable).
// waitForData() tells you when there is actually banner data ready to recv() (socket becomes readable).
// Using waitForConnection() inside grabBanner() was the root cause of the hang, because a freshly
// connected socket is already writable almost instantly, regardless of whether the server has sent
// anything yet.
bool waitForData(socket_t sock, int timeoutSeconds)
{
    fd_set readSet, errorSet;
    FD_ZERO(&readSet);
    FD_ZERO(&errorSet);
    FD_SET(sock, &readSet);
    FD_SET(sock, &errorSet);

    struct timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;

    int result = select(sock + 1, &readSet, nullptr, &errorSet, &timeout);
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

// these are the new functiond over the 3rd level port scanner

std::string getProbeForPort(int port)
{
    auto it = serviceMap.find(port);
    // searching for port in the service header file

    if (it != serviceMap.end())
    {
        return it->second.probe;
        // if the port is found then return the probe string
        // structure of the servicemap(std::map<int, ServiceInfo> serviceMap)
    }

    return "";
}

std::string grabBanner(socket_t sock, int port)
{
    std::string probe = getProbeForPort(port);

    {
        // FIX: wrapped debug prints in printMutex so they don't interleave across threads
        std::lock_guard<std::mutex> lock(printMutex);
        cout << "Port " << port << " probe: ";
        if (probe.empty())
        {
            cout << "PASSIVE (no probe)" << endl;
        }
        else
        {
            cout << "ACTIVE (sending probe)" << endl;
        }
    }

    // Send probe if needed
    if (!probe.empty())
    {
        int sent = send(sock, probe.c_str(), probe.length(), 0);
        {
            std::lock_guard<std::mutex> lock(printMutex);
            cout << "  Sent: " << sent << " bytes" << endl;
        }
        if (sent < 0)
        {
            return "";
        }
    }

    // Wait for response
    // FIX: switched from waitForConnection() to waitForData() — we are waiting for
    // bytes to arrive (readability), not waiting for the connect handshake (writability).
    if (waitForData(sock, 2))
    {
        {
            std::lock_guard<std::mutex> lock(printMutex);
            cout << "  Select returned - data ready" << endl;
        }

        // FIX: removed the switch back to blocking mode. The socket stays non-blocking,
        // so recv() below can never hang indefinitely — waitForData() already confirmed
        // (with its own 2 second timeout) that data is present before we get here.
        char buffer[1024];
        int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);

        {
            std::lock_guard<std::mutex> lock(printMutex);
            cout << "  Received: " << bytesRead << " bytes" << endl;
        }

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            {
                std::lock_guard<std::mutex> lock(printMutex);
                cout << "  Banner: " << buffer << endl;
            }
            return std::string(buffer);
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(printMutex);
        cout << "  Select timeout (no data)" << endl;
    }

    return "";
}

std::string identifyService(int port, std::string banner)
{
    // looking for the service in port database included header file
    auto it = serviceMap.find(port);

    // if the port is getting matched with a entry in the servicemap , return the service name
    if (it != serviceMap.end())
    {
        return it->second.name;
    }

    // if the banner is empty , we will just return unknown
    if (banner.empty())
    {
        return "Unknown";
    }

    // if the port number is not matching with the entries in servicemap , we will check it by hardcoding the patterns
    if (banner.find("SSH-") != std::string::npos)
    {
        return "SSH";
    }

    if (banner.find("HTTP/") != std::string::npos)
    {
        return "HTTP";
    }

    if (banner.find("MySQL") != std::string::npos)
    {
        return "MySQL";
    }

    if (banner.find("220") == 0)
    {
        return "SMTP/FTP";
    }

    if (banner.find("PostgreSQL") != std::string::npos)
    {
        return "PostgreSQL";
    }

    if (banner.find("RFB") != std::string::npos)
    {
        return "VNC";
    }

    if (banner.find("REDIS") != std::string::npos)
    {
        return "Redis";
    }

    if (banner.find("Elasticsearch") != std::string::npos)
    {
        return "Elasticsearch";
    }

    if (banner.find("MongoDB") != std::string::npos)
    {
        return "MongoDB";
    }

    // if the service cannot be identified i will return the banner
    std::string cleanBanner = banner;
    if (cleanBanner.length() > 100)
    {
        cleanBanner = cleanBanner.substr(0, 100) + "...";
    }

    return "Unknown: " + cleanBanner;
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
            // if the control goes to this block then the port is open .Now we can proceed with banner grabbing
            std::string banner = grabBanner(sock, port);
            // now the deatils about the banner and service is stored in the variable banner
            std::string service = identifyService(port, banner); // now we need to identify the service with the banner found

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

        {
            // FIX: wrapped in printMutex so full lines print atomically, no more
            // "Port Port 1003 is FILTERED" style interleaving
            std::lock_guard<std::mutex> lock(printMutex);
            cout << "Port " << port << " is " << status << endl;
        }

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

    // main thread will wait for the other threads to finish to go into printing the results
    //w emust ise the 
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
