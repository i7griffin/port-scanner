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
