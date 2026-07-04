# port-scanner
# Phase 1: Single-Port TCP Connect Check

## Goal

Build a C++ program that accepts one IP address and one port number, attempts to establish a TCP connection to that address/port combination, and reports whether the connection succeeded (port open) or failed (port closed/unreachable). This phase establishes the foundational socket programming concepts and cross-platform compatibility infrastructure needed for all subsequent phases.

## Core Concepts

**TCP Three-Way Handshake**

TCP connections begin with a three-step handshake before any data transfers:

1. Your machine sends a SYN (synchronization) packet to initiate the connection
2. The remote machine replies with a SYN-ACK packet if something is listening on that port
3. Your machine sends an ACK (acknowledgment) packet to complete the handshake

The connection is now established and ready for data transfer. If nothing is listening on the target port, the remote OS immediately responds with a RST (reset) packet, rejecting the connection.

The `connect()` function abstracts away these packet exchanges. Your program calls `connect()` once, and the kernel handles the entire three-way handshake internally. The function returns only after the handshake completes (success) or fails definitively (port closed/unreachable/filtered).

**Sockets**

A socket is an OS-level abstraction representing one endpoint of a network connection. On the system level, it's simply a small integer (a file descriptor on Linux, a handle on Windows) that the OS uses as a reference. Your program never directly accesses the socket's internal state — it only uses this handle to tell the kernel what operations to perform.

The OS maintains the actual socket state (buffers, connection parameters, protocol state, etc.) in kernel memory. When you call `socket()`, the kernel allocates these internal structures and returns you the handle. When you call functions like `connect()` or `close()`, you pass the handle so the kernel knows which socket to operate on.

**Network Byte Order**

CPUs store multi-byte numbers (integers, addresses, port numbers) in different orders depending on the architecture. Most modern CPUs use little-endian order (least significant byte first), but network protocols standardize on big-endian order (most significant byte first), called network byte order.

Port numbers are 16-bit values. In little-endian (your CPU), port 22 is stored as `[0x16, 0x00]` in memory. In network byte order, it's `[0x00, 0x16]`. If you send the port number in your machine's native byte order without converting, the remote machine interprets it as a different port entirely.

The `htons()` function ("host to network short") performs this conversion for 16-bit values like ports. The kernel handles byte order conversion automatically for IP addresses via `inet_pton()`, but you must explicitly call `htons()` for port numbers.

**Address Structures**

The `sockaddr_in` struct bundles an IPv4 address, port, and protocol family into a single data structure in the exact format the OS kernel expects. It is defined as:

```cpp
struct sockaddr_in {
    short          sin_family;   // Protocol family (AF_INET for IPv4)
    unsigned short sin_port;     // Port number in network byte order
    struct in_addr sin_addr;     // IPv4 address in binary form
    char           sin_zero[8];  // Padding, must be zero
};
```

The `in_addr` struct nested within `sockaddr_in` contains a single 32-bit unsigned integer representing the IPv4 address in binary form.

The `sin_zero` padding field is required for compatibility with the generic `sockaddr` type, which is larger. This field must be explicitly zeroed — using uninitialized padding can cause unpredictable behavior on some systems. The standard practice is to zero the entire struct with `memset()` before filling in the named fields.

**Cross-Platform Socket Types**

Different operating systems represent sockets differently:
- On Linux, a socket is a plain `int` (file descriptor)
- On Windows, a socket is a `SOCKET` type (unsigned integer handle)

An invalid socket is:
- `-1` on Linux
- `INVALID_SOCKET` constant on Windows

To write code that compiles identically on both platforms, use a type alias and constant that map to the correct platform-specific types:

```cpp
#ifdef _WIN32
    using socket_t = SOCKET;
    const socket_t INVALID_SOCK = INVALID_SOCKET;
#else
    using socket_t = int;
    const socket_t INVALID_SOCK = -1;
#endif
```

All socket variables then use `socket_t`, and the preprocessor automatically selects the correct underlying type for each platform.

**Windows Winsock Initialization**

On Linux, sockets are a core kernel feature always available — no initialization is needed. On Windows, the networking layer (Winsock) is a separate component that must be explicitly initialized before any socket function can be used.

The `WSAStartup()` function initializes Winsock, and `WSACleanup()` releases it. Calling any socket function without first calling `WSAStartup()` results in error `WSANOTINITIALISED`.

Linux has no equivalent to these functions — sockets work immediately. Wrap both in `initSockets()` and `cleanupSockets()` functions with `#ifdef` guards so `main()` remains platform-agnostic.

**Error Reporting**

Platform differences also extend to error reporting:
- On Linux, socket function failures set the global `errno` variable. Use `strerror(errno)` to get a human-readable error message.
- On Windows, socket function failures call `WSAGetLastError()` to retrieve the error code. There is no direct equivalent to `strerror()` without using more complex APIs.

Wrap both in a `getSocketError()` function that returns a string, allowing the rest of the program to report errors without platform-specific code.

## Key Functions You Used

**`socket(AF_INET, SOCK_STREAM, 0)`** — Creates a new socket. Takes the address family (`AF_INET` for IPv4), socket type (`SOCK_STREAM` for TCP), and protocol (0 lets the OS select). Returns a socket handle on success, `INVALID_SOCK` on failure. This must be called before any other socket operations.

**`htons(port)`** — Converts a 16-bit port number from host byte order to network byte order. Essential before placing the port into `sin_port`. Omitting this is a classic bug that silently connects to the wrong port.

**`inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr)`** — Converts a human-readable IPv4 address string into its 32-bit binary representation. Takes the address family, the string, and a pointer to the destination. Returns 1 on success, 0 if the string is not a valid IP address, -1 on other errors. Automatically handles byte order conversion, so no separate `htonl()` call is needed.

**`memset(&addr, 0, sizeof(addr))`** — Fills the address struct with zeros. Essential before filling in the named fields, as it ensures the padding (`sin_zero`) is properly initialized. Some systems have undefined behavior if padding is uninitialized.

**`connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr))`** — Initiates the TCP three-way handshake to the address specified in the `sockaddr_in` struct. The second parameter is cast to the generic `sockaddr*` type the function signature expects (memory-compatible cast). Returns 0 on successful connection (port open), -1 on failure (port closed/filtered/unreachable). Blocks until the handshake completes or the OS timeout expires (typically 20–60 seconds).

**`WSAStartup(MAKEWORD(2, 2), &wsaData)`** (Windows only) — Initializes Winsock. The macro `MAKEWORD(2, 2)` specifies version 2.2 (the current standard). The `wsaData` struct receives information about the Winsock implementation. Returns 0 on success. Must be called before any other socket function on Windows.

**`WSAGetLastError()`** (Windows) / **`strerror(errno)`** (Linux) — Retrieves a human-readable error description after a socket function fails. Windows and Linux use different mechanisms, so wrap both in a helper function.

**`closesocket()`** (Windows) / **`close()`** (Linux) — Releases the socket handle back to the OS. Failure to call this leaks file descriptors. Must be called even in error paths to ensure cleanup.

## The Program Structure

```
main()
├─ initSockets()                           // Windows: WSAStartup, Linux: nothing
├─ Get IP address from user (string input)
├─ Get port number from user (int input)
├─ Declare and zero sockaddr_in struct
├─ Fill address struct:
│  ├─ sin_family = AF_INET
│  ├─ sin_port = htons(port)
│  └─ inet_pton(AF_INET, ip, &addr.sin_addr)
├─ Check inet_pton() result:
│  ├─ Fail → Print error and exit early
│  └─ Success → Continue
├─ createsocket()
├─ Check socket creation:
│  ├─ Fail → Print error and exit early
│  └─ Success → Continue
├─ connect(socket, &addr, sizeof(addr))
├─ Check result:
│  ├─ 0 → Print "Port X is OPEN"
│  └─ -1 → Print "Port X is CLOSED"
├─ closeSocket(socket)
├─ cleanupSockets()                        // Windows: WSACleanup, Linux: nothing
└─ return 0
```

## How to Compile and Run

**Command line (PowerShell on Windows):**
```powershell
g++ -o build/scanner src/main.cpp -lws2_32
.\build\scanner
```

**Command line (Bash on Linux):**
```bash
g++ -o build/scanner src/main.cpp
./build/scanner
```

**With Makefile:**
```powershell
make
.\build\scanner
```

The `-lws2_32` flag (Windows only) links against the Winsock library. This tells the linker where to find the compiled implementations of socket functions. The Makefile can conditionally include this flag based on platform detection.

## Testing Phase 1

**Identify open ports on your machine:**
```powershell
netstat -ano | findstr LISTENING
```

This lists ports where services are actively listening, suitable for testing the success case.

**Test with a known open port:**
```
Enter IP: 127.0.0.1
Enter port: [any port from netstat output, e.g., 22 if SSH is running]
Expected output: Port 22 is OPEN
```

**Test with a known closed port:**
```
Enter IP: 127.0.0.1
Enter port: 9999 (assuming nothing listens here)
Expected output: Port 9999 is CLOSED
```

**Important observation:** If the program hangs for 20+ seconds before printing "CLOSED," you have hit a filtered port (a firewall silently drops packets instead of rejecting them). This behavior is expected in Phase 1 and is the problem Phase 2 solves with custom timeouts.

## What Phase 1 Does NOT Do

- **No port ranges** — Only checks a single port per program execution
- **No concurrency** — Sequential execution; scales poorly
- **No custom timeouts** — Uses the OS default (typically 20–60 seconds), causing long hangs on filtered ports
- **No service identification** — Reports only open/closed, not what service is running
- **No command-line parsing** — Requires user input during execution
- **No output options** — Results print only to console
- **No DNS resolution** — Requires an IP address, not a hostname

## Performance Characteristics

- **Open port:** ~0.1 seconds (instant kernel confirmation)
- **Closed port:** ~0.1–0.2 seconds (kernel receives and processes RST)
- **Filtered port:** 20–60+ seconds (OS timeout waiting for response that never comes)

## Phase 1 Checkpoint

Phase 1 is complete when:
- Program successfully compiles on both Windows and Linux
- Correctly reports ports as OPEN or CLOSED against tested ports
- Properly cleans up resources (sockets are closed)
- Accepts user input for IP and port
- Handles invalid IP addresses gracefully
- Handles socket creation failures gracefully
- Compiles and runs without platform-specific code in `main()`

# Phase 2: Port Range + Timeout Handling

## Goal

Extend the single-port scanner to scan a range of ports (e.g., 1–1024) sequentially, with custom timeout control. Eliminate indefinite hangs on filtered ports by using non-blocking sockets and `select()` to wait a bounded time (1–2 seconds per port) instead of the OS default (20–60+ seconds).

## Core Concepts

**Non-Blocking Sockets**

A socket created in Phase 1 is blocking — when you call `connect()`, the function doesn't return until the connection either succeeds, fails with a clear error, or times out using the OS's default timeout (often 20–60 seconds). This is problematic for port scanning: a single filtered port can stall the entire scan for over a minute.

A non-blocking socket returns immediately from `connect()`, even if the connection isn't complete. The actual connection attempt continues in the background, managed by the OS kernel. You then check on the connection's status later using `select()`, with your own timeout.

On Windows, non-blocking is enabled via `ioctlsocket(sock, FIONBIO, &mode)` where `mode = 1`.
On Linux, non-blocking is enabled via `fcntl(sock, F_SETFL, flags | O_NONBLOCK)`.

**The select() Function**

`select()` is a blocking call that waits for network activity on one or more sockets, with a timeout you specify. You pass it:
- One or more file descriptor sets (collections of sockets)
- A timeout duration (seconds and microseconds)

`select()` blocks until one of three things happens:
1. Activity occurs on one of the watched sockets (it returns immediately with the count of active sockets)
2. The timeout expires (it returns 0)
3. An error occurs (it returns -1)

For a port scanner, you watch for two conditions simultaneously on each socket: write-ready (successful connection) or error-ready (failed connection). You create two fd_sets (`writeSet` and `errorSet`), add the socket to both, and call `select()` with a 1–2 second timeout.

**File Descriptor Sets (fd_set)**

An `fd_set` is a data structure representing a collection of file descriptors (sockets on Windows/Linux). You manipulate it using macros:
- `FD_ZERO(&set)` — clear the set (make it empty)
- `FD_SET(sock, &set)` — add a socket to the set
- `FD_CLR(sock, &set)` — remove a socket from the set

After `select()` returns, you check which sockets are in which sets using `FD_ISSET(sock, &set)` to determine what happened.

**Checking Connection Results with getsockopt()**

After `select()` indicates a socket is ready, you call `getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &errorLen)` to read the actual error code stored on the socket. This tells you:
- `error == 0` → connection succeeded (port is OPEN)
- `error == ECONNREFUSED` → connection was refused (port is CLOSED)
- Other error codes → connection failed for other reasons (port is FILTERED or unreachable)

**The Timeout Problem and Solution**

Phase 1 can hang for 20–60+ seconds per port on filtered ports because `connect()` uses the OS default TCP timeout. A 1024-port scan where half the ports are filtered could take 30+ minutes.

Phase 2 solves this by:
1. Setting sockets non-blocking so `connect()` returns immediately
2. Using `select()` with a custom 1–2 second timeout
3. If `select()` times out, moving to the next port immediately
4. If `select()` returns, checking the actual result and moving on

A full 1–1024 port scan now completes in seconds regardless of filtered ports.

## Key Functions You Used

**`setNonBlocking(socket_t sock)`** — Sets a socket to non-blocking mode. On Windows calls `ioctlsocket()`, on Linux calls `fcntl()`. After this, `connect()` returns immediately instead of blocking indefinitely.

**`waitForConnection(socket_t sock, int timeoutSeconds)`** — Creates two fd_sets (writeSet and errorSet), adds the socket to both, and calls `select()` with a timeout. Returns true if the socket is ready (connection completed, either succeeded or failed), false if timeout expired.

**`checkConnectionResult(socket_t sock)`** — Calls `getsockopt(SO_ERROR)` to read the actual result from the socket. Returns true if error is 0 (port open), false otherwise (port closed/filtered).

**`htons(port)`** — Converts port number to network byte order. Called on each port in the loop before updating `addr.sin_port`.

**`connect(sock, &addr, sizeof(addr))`** — Same as Phase 1, but now returns immediately on non-blocking sockets with error EINPROGRESS or EWOULDBLOCK (meaning "connection in progress").

## The Program Structure

```
main()
├─ initSockets()                           // Windows: WSAStartup
├─ Get IP from user
├─ Get port range (startPort, endPort)
├─ Set up address struct (once)
│  ├─ memset
│  ├─ sin_family = AF_INET
│  └─ inet_pton() for IP
│
├─ Loop over port range:
│  ├─ createsocket()
│  ├─ setNonBlocking(sock)
│  ├─ Update addr.sin_port = htons(port)
│  ├─ connect(sock, &addr)
│  │  └─ Returns immediately with EINPROGRESS
│  ├─ waitForConnection(sock, 2 seconds)
│  │  ├─ True → checkConnectionResult()
│  │  │         ├─ True → Print "OPEN"
│  │  │         └─ False → Print "CLOSED"
│  │  └─ False (timeout) → Print "FILTERED"
│  └─ closeSocket(sock)
│
├─ cleanupSockets()                        // Windows: WSACleanup
└─ return 0
```

## How to Compile and Run

Command line (PowerShell on Windows):
```powershell
g++ -o build/scanner src/main.cpp -lws2_32
.\build\scanner
```

Then enter:
- IP address (e.g., `127.0.0.1`)
- Start port (e.g., `1`)
- End port (e.g., `1024`)

With Makefile:
```powershell
make
.\build\scanner
```

The `-lws2_32` flag links against the Windows Winsock library (same as Phase 1).

## Testing Phase 2

**Scan a small range on localhost:**
```
IP: 127.0.0.1
Start port: 1
End port: 1024
```

Expected behavior:
- Scan completes in 3–5 seconds (not 20+ minutes)
- Shows a mix of OPEN, CLOSED, and FILTERED ports
- Example output:
  ```
  Port 135 is OPEN
  Port 137 is CLOSED
  Port 445 is OPEN
  Port 1 is FILTERED
  Port 2 is FILTERED
  ...
  ```

**Verify timeout is working:**

Add debug output (print start and end time) to confirm the scan completes in a predictable time regardless of how many ports return no response.

**Confirm resource cleanup:**

Run the scan multiple times back-to-back. If the program leaks file descriptors, it will eventually fail with "too many open files." Proper cleanup (close socket every iteration) should allow unlimited consecutive scans.

## What Phase 2 Does NOT Do

- **No threading** — Still scans one port at a time sequentially. Phase 3 adds this.
- **No banner grabbing** — Still only reports open/closed, not service identification.
- **No DNS resolution** — Still requires an IP address, not a hostname.
- **No output to file** — Results only print to console.
- **No scanning statistics** — No timing information or summary reports.
- **No port name mapping** — No display of common service names (SSH, HTTP, etc.) associated with port numbers.

## Performance Characteristics

- **Single port (Phase 1):** ~0.1 seconds (instant success/failure) to 2 seconds (filtered port with timeout)
- **1024 ports (Phase 2, sequential):** ~10–20 seconds with many responsive ports, up to ~2000 seconds (if timeout is very long) with many filtered ports. Typical local scan: 3–5 seconds.
- **Threading benefit (Phase 3):** 1024 ports with 50 workers: ~5–10 seconds regardless of responsiveness, since 50 ports are scanned in parallel.

## Phase 2 Checkpoint

Phase 2 is complete when:
- Scans a configurable port range without hanging
- Completes in predictable time (seconds, not minutes)
- Correctly identifies open, closed, and filtered ports
- Properly closes all sockets (no resource leaks)
- Works on both Windows and Linux with same code
