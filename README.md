# port-scanner
# Phase 1 : Single-Port TCP Connect Check

## Goal
Build a C++ program that takes one IP address and one port number, attempts to open a TCP connection to that address/port, and reports whether the connection succeeded (port open) or failed (port closed/unreachable).

### Core Concepts

**TCP Three-Way Handshake:** Before data flows, TCP requires a handshake:
- Your machine sends SYN (synchronize request)
- Remote machine replies with SYN-ACK (acknowledge + my own SYN)
- Your machine sends ACK (acknowledge their response)
- Connection is now open

The `connect()` function handles all of this internally — you don't see the packets, just the success/failure result.

**Sockets:** A socket is an OS-level handle (a small integer) representing one end of a network connection. It's not the connection itself, but a reference to it. The OS manages the actual data structures and buffers; your program just uses the handle to tell the OS what to do.

**Network Byte Order:** Multi-byte numbers (like port numbers) are stored in different byte orders depending on your CPU. Networks expect "network byte order" (big-endian). `htons()` converts from your machine's order to network order. Without it, your port number gets sent backwards and connects to the wrong port.

**Address Structures:** `sockaddr_in` is a predefined struct that bundles an IPv4 address, port, and protocol family together in the exact format the OS expects. It has four fields:
- `sin_family` — address family (AF_INET for IPv4)
- `sin_port` — port in network byte order (set via `htons()`)
- `sin_addr` — IP address in binary form (set via `inet_pton()`)
- `sin_zero` — padding, kept zeroed

**Cross-Platform Socket Types:** A socket is an `int` on Linux but `SOCKET` on Windows. Using a `socket_t` type alias lets the same code work on both:
```cpp
#ifdef _WIN32
    using socket_t = SOCKET;
    const socket_t INVALID_SOCK = INVALID_SOCKET;
#else
    using socket_t = int;
    const socket_t INVALID_SOCK = -1;
#endif
```

**Windows Winsock Initialization:** Windows requires `WSAStartup()` before any socket function works. Linux doesn't need this. Wrap it in a `initSockets()` function so `main()` stays platform-agnostic.

### Key Functions You Used

**`socket(AF_INET, SOCK_STREAM, 0)`** — Creates a socket. Returns a handle (int or SOCKET) on success, INVALID_SOCK on failure. Must be called before anything else networking-related.

**`htons(port)`** — Converts a port number to network byte order. Essential before putting the port into `sin_port`. Without it, you silently connect to the wrong port.

**`inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr)`** — Converts an IP string to binary form. Returns 1 on success, 0 if the string isn't a valid IP, -1 on other errors. Writes the result directly into the struct's `sin_addr` field.

**`connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr))`** — Initiates the TCP three-way handshake. Returns 0 on success (port open), -1 on failure (port closed/filtered/unreachable). Blocks until the handshake completes or times out.

**`WSAStartup(MAKEWORD(2,2), &wsaData)`** (Windows only) — Initializes Winsock. Must be called once before using any socket functions on Windows. Returns 0 on success.

**`WSAGetLastError()`** (Windows) vs **`strerror(errno)`** (Linux) — Get human-readable error descriptions after a function fails. Windows and Linux use different error reporting mechanisms, so wrap both in a `getSocketError()` function.

**`closesocket()`** (Windows) vs **`close()`** (Linux) — Releases the socket handle back to the OS. Must be called to avoid resource leaks, even on error paths.

## The Program Structure

```
main()
├─ initSockets()                    // Windows: WSAStartup, Linux: nothing
├─ Get IP from user
├─ Get port from user
├─ createsocket()                   // Create a socket handle
├─ memset(&addr, 0, sizeof(addr))   // Zero the address struct
├─ Fill sockaddr_in:
│  ├─ addr.sin_family = AF_INET
│  ├─ addr.sin_port = htons(port)
│  └─ inet_pton(AF_INET, ip, &addr.sin_addr)
├─ connect(socket, address)         // Attempt the handshake
├─ Check result:
│  ├─ 0 → Port OPEN
│  └─ -1 → Port CLOSED
├─ closeSocket(socket)              // Clean up
├─ cleanupSockets()                 // Windows: WSACleanup, Linux: nothing
└─ return 0
```

## How to Compile and Run

**Command line (PowerShell on Windows):**
```powershell
g++ -o build/scanner src/main.cpp -lws2_32
.\build\scanner
```

Then enter an IP address (e.g., `127.0.0.1`) and a port number (e.g., `22`).

**With Makefile (if set up):**
```powershell
make
.\build\scanner
```

**What `-lws2_32` means:** Link against the Windows Winsock library, which contains the compiled code for socket functions. Without it, the linker can't find where `socket()`, `connect()`, etc. actually live.

## Testing Phase 1

**Find open ports on your machine:**
```powershell
netstat -ano | findstr LISTENING
```

**Test with a known open port:**
```
IP: 127.0.0.1
Port: [any port from netstat output]
Expected output: Port [X] is OPEN
```

**Test with a known closed port:**
```
IP: 127.0.0.1
Port: 9999  (or any unused high port)
Expected output: Port 9999 is CLOSED
```

**Important:** If the program hangs for 20+ seconds before saying "CLOSED," you've hit a filtered port (firewall silently drops packets). This is the exact problem Phase 2 solves with timeout handling.

## What Phase 1 Does NOT Do

- No port ranges — only checks one port per run
- No threading — no concurrency, so it's slow at scale
- No timeout control — uses the OS default (often 20-60 seconds)
- No banner grabbing — just reports open/closed, not what's running
- No output options — just prints to screen

## What's Next (Phase 2)

Phase 2 adds **custom timeout handling** using non-blocking sockets and `select()`. This solves the hanging problem: instead of waiting 60+ seconds on a filtered port, your scanner will wait only 1-2 seconds and move on, making full port scans actually usable.

## Key Takeaways

1. A socket is just a handle; the OS manages the real connection state
2. `htons()` and `inet_pton()` handle the messy byte-order conversions — don't skip them
3. Windows and Linux have completely different socket APIs beneath the surface — type aliases and `#ifdef` blocks isolate these differences so your logic stays clean
4. `connect()` does all the handshake work for you; you just check if it succeeded
5. Resource cleanup (close, WSACleanup) matters, even when errors occur
6. The default TCP timeout is very long — Phase 2 adds custom timeouts to make scanning practical
