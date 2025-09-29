#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#pragma comment(lib, "ws2_32.lib")

volatile int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
    printf("\nInterrupt received. Exiting gracefully...\n");
}

void print_usage(const char* prog) {
    printf("Usage: %s <host> <port> [options]\n", prog);
    printf("\nRequired:\n");
    printf("  <host>                Target IP address or hostname\n");
    printf("  <port>                Target port number\n");
    printf("\nSocket Options (all optional):\n");
    printf("  --keepalive <0|1>     Enable/disable TCP keepalive (SO_KEEPALIVE)\n");
    printf("  --keepalive-time <ms> Time before sending keepalive probe (TCP_KEEPALIVE, SIO_KEEPALIVE_VALS)\n");
    printf("  --keepalive-interval <ms> Interval between keepalive probes (SIO_KEEPALIVE_VALS)\n");
    printf("  --nodelay <0|1>       Enable/disable Nagle's algorithm (TCP_NODELAY)\n");
    printf("  --maxseg <bytes>      Set maximum segment size (TCP_MAXSEG)\n");
    printf("  --connect-timeout <ms> Connection timeout in milliseconds\n");
    printf("  --send-timeout <ms>   Send timeout (SO_SNDTIMEO)\n");
    printf("  --recv-timeout <ms>   Receive timeout (SO_RCVTIMEO)\n");
    printf("  --sendbuf <bytes>     Send buffer size (SO_SNDBUF)\n");
    printf("  --recvbuf <bytes>     Receive buffer size (SO_RCVBUF)\n");
    printf("  --linger <sec>        Linger time on close, -1 to disable (SO_LINGER)\n");
    printf("  --maxrt <sec>         Maximum retransmission timeout in seconds (TCP_MAXRT)\n");
    printf("\nNote: TCP_MAXRT sets max retransmission timeout (time-based), not retry count.\n");
    printf("      Windows does not expose per-socket retry count configuration.\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    // Set up signal handler for clean exit
    signal(SIGINT, int_handler);

    const char* host = argv[1];
    const char* port = argv[2];

    // Parse optional arguments
    int opt_keepalive = -1;
    DWORD opt_keepalive_time = 0;
    DWORD opt_keepalive_interval = 0;
    int opt_nodelay = -1;
    int opt_maxseg = -1;
    int opt_connect_timeout = 0;
    int opt_send_timeout = -1;
    int opt_recv_timeout = -1;
    int opt_sendbuf = -1;
    int opt_recvbuf = -1;
    int opt_linger = -2; // -2 = not set, -1 = disable, >=0 = seconds
    int opt_maxrt = -1; // Maximum retransmission timeout in seconds

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--keepalive") == 0 && i + 1 < argc) {
            opt_keepalive = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--keepalive-time") == 0 && i + 1 < argc) {
            opt_keepalive_time = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--keepalive-interval") == 0 && i + 1 < argc) {
            opt_keepalive_interval = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--nodelay") == 0 && i + 1 < argc) {
            opt_nodelay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--maxseg") == 0 && i + 1 < argc) {
            opt_maxseg = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--connect-timeout") == 0 && i + 1 < argc) {
            opt_connect_timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--send-timeout") == 0 && i + 1 < argc) {
            opt_send_timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--recv-timeout") == 0 && i + 1 < argc) {
            opt_recv_timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--sendbuf") == 0 && i + 1 < argc) {
            opt_sendbuf = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--recvbuf") == 0 && i + 1 < argc) {
            opt_recvbuf = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--linger") == 0 && i + 1 < argc) {
            opt_linger = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--maxrt") == 0 && i + 1 < argc) {
            opt_maxrt = atoi(argv[++i]);
        }
    }

    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    // Resolve address
    struct addrinfo hints = {0}, *addr_result = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(host, port, &hints, &addr_result);
    if (result != 0) {
        printf("getaddrinfo failed: %d\n", result);
        WSACleanup();
        return 1;
    }

    // Create socket
    SOCKET sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        freeaddrinfo(addr_result);
        WSACleanup();
        return 1;
    }

    printf("Socket created successfully\n");

    // Apply socket options BEFORE connecting
    
    if (opt_keepalive >= 0) {
        DWORD val = opt_keepalive;
        if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set SO_KEEPALIVE: %d\n", WSAGetLastError());
        } else {
            printf("Set SO_KEEPALIVE = %d\n", val);
        }
    }

    if (opt_keepalive_time > 0 || opt_keepalive_interval > 0) {
        struct tcp_keepalive keepalive_vals;
        keepalive_vals.onoff = 1;
        keepalive_vals.keepalivetime = opt_keepalive_time > 0 ? opt_keepalive_time : 7200000;
        keepalive_vals.keepaliveinterval = opt_keepalive_interval > 0 ? opt_keepalive_interval : 1000;
        
        DWORD bytes_returned;
        if (WSAIoctl(sock, SIO_KEEPALIVE_VALS, &keepalive_vals, sizeof(keepalive_vals),
                     NULL, 0, &bytes_returned, NULL, NULL) == SOCKET_ERROR) {
            printf("Failed to set SIO_KEEPALIVE_VALS: %d\n", WSAGetLastError());
        } else {
            printf("Set keepalive time=%dms, interval=%dms\n", 
                   keepalive_vals.keepalivetime, keepalive_vals.keepaliveinterval);
        }
    }

    if (opt_nodelay >= 0) {
        DWORD val = opt_nodelay;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set TCP_NODELAY: %d\n", WSAGetLastError());
        } else {
            printf("Set TCP_NODELAY = %d\n", val);
        }
    }

    if (opt_maxseg > 0) {
        DWORD val = opt_maxseg;
        if (setsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set TCP_MAXSEG: %d\n", WSAGetLastError());
        } else {
            printf("Set TCP_MAXSEG = %d\n", val);
        }
    }

    if (opt_send_timeout >= 0) {
        DWORD val = opt_send_timeout;
        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set SO_SNDTIMEO: %d\n", WSAGetLastError());
        } else {
            printf("Set SO_SNDTIMEO = %dms\n", val);
        }
    }

    if (opt_recv_timeout >= 0) {
        DWORD val = opt_recv_timeout;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set SO_RCVTIMEO: %d\n", WSAGetLastError());
        } else {
            printf("Set SO_RCVTIMEO = %dms\n", val);
        }
    }

    if (opt_sendbuf > 0) {
        int val = opt_sendbuf;
        if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set SO_SNDBUF: %d\n", WSAGetLastError());
        } else {
            printf("Set SO_SNDBUF = %d bytes\n", val);
        }
    }

    if (opt_recvbuf > 0) {
        int val = opt_recvbuf;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set SO_RCVBUF: %d\n", WSAGetLastError());
        } else {
            printf("Set SO_RCVBUF = %d bytes\n", val);
        }
    }

    if (opt_linger >= -1) {
        struct linger ling;
        if (opt_linger == -1) {
            ling.l_onoff = 0;
            ling.l_linger = 0;
        } else {
            ling.l_onoff = 1;
            ling.l_linger = opt_linger;
        }
        if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling)) == SOCKET_ERROR) {
            printf("Failed to set SO_LINGER: %d\n", WSAGetLastError());
        } else {
            printf("Set SO_LINGER = %s (linger=%d sec)\n", 
                   ling.l_onoff ? "enabled" : "disabled", ling.l_linger);
        }
    }

    if (opt_maxrt >= 0) {
        DWORD val = opt_maxrt;
        if (setsockopt(sock, IPPROTO_TCP, TCP_MAXRT, (char*)&val, sizeof(val)) == SOCKET_ERROR) {
            printf("Failed to set TCP_MAXRT: %d\n", WSAGetLastError());
        } else {
            printf("Set TCP_MAXRT = %d seconds\n", val);
        }
    }

    // Set non-blocking mode for connect timeout
    if (opt_connect_timeout > 0) {
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
    }

    // Connect
    printf("\nConnecting to %s:%s...\n", host, port);
    result = connect(sock, addr_result->ai_addr, (int)addr_result->ai_addrlen);
    
    if (opt_connect_timeout > 0) {
        if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set write_fds, except_fds;
            FD_ZERO(&write_fds);
            FD_ZERO(&except_fds);
            FD_SET(sock, &write_fds);
            FD_SET(sock, &except_fds);
            
            struct timeval tv;
            tv.tv_sec = opt_connect_timeout / 1000;
            tv.tv_usec = (opt_connect_timeout % 1000) * 1000;
            
            result = select(0, NULL, &write_fds, &except_fds, &tv);
            if (result == 0) {
                printf("Connection timeout after %dms\n", opt_connect_timeout);
                closesocket(sock);
                freeaddrinfo(addr_result);
                WSACleanup();
                return 1;
            } else if (result == SOCKET_ERROR || FD_ISSET(sock, &except_fds)) {
                printf("Connection failed during timeout: %d\n", WSAGetLastError());
                closesocket(sock);
                freeaddrinfo(addr_result);
                WSACleanup();
                return 1;
            }
            // Connection succeeded, set back to blocking mode
            u_long mode = 0;
            ioctlsocket(sock, FIONBIO, &mode);
        } else if (result == SOCKET_ERROR) {
            printf("connect() failed: %d\n", WSAGetLastError());
            closesocket(sock);
            freeaddrinfo(addr_result);
            WSACleanup();
            return 1;
        }
    } else if (result == SOCKET_ERROR) {
        printf("connect() failed: %d\n", WSAGetLastError());
        closesocket(sock);
        freeaddrinfo(addr_result);
        WSACleanup();
        return 1;
    }

    printf("Connected successfully!\n\n");

    // Read and display current socket options
    printf("Current socket option values:\n");
    
    int intval;
    int optlen = sizeof(intval);
    if (getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&intval, &optlen) == 0) {
        printf("  SO_KEEPALIVE: %d\n", intval);
    }
    
    if (getsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&intval, &optlen) == 0) {
        printf("  TCP_NODELAY: %d\n", intval);
    }

    if (getsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char*)&intval, &optlen) == 0) {
        printf("  TCP_MAXSEG: %d\n", intval);
    }

    if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&intval, &optlen) == 0) {
        printf("  SO_SNDBUF: %d bytes\n", intval);
    }

    if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&intval, &optlen) == 0) {
        printf("  SO_RCVBUF: %d bytes\n", intval);
    }

    struct linger ling;
    optlen = sizeof(ling);
    if (getsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&ling, &optlen) == 0) {
        printf("  SO_LINGER: %s (linger=%d sec)\n", 
               ling.l_onoff ? "enabled" : "disabled", ling.l_linger);
    }

    DWORD maxrt;
    optlen = sizeof(maxrt);
    if (getsockopt(sock, IPPROTO_TCP, TCP_MAXRT, (char*)&maxrt, &optlen) == 0) {
        printf("  TCP_MAXRT: %d seconds\n", maxrt);
    }

    char buffer[4096];
    while (keep_running) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("[recv] %d bytes: %s\n", bytes, buffer);
        } else if (bytes == 0) {
            printf("Connection closed by peer.\n");
            break;
        } else {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAEINTR) {
                Sleep(1); // no data, wait a bit
            } else {
                printf("recv() failed: %d\n", err);
                break;
            }
        }
        // Send a simple test message
        const char* msg = "GET / HTTP/1.0\r\n\r\n";
        printf("\nSending test HTTP request...\n");
        result = send(sock, msg, (int)strlen(msg), 0);
        if (result == SOCKET_ERROR) {
            printf("send() failed: %d\n", WSAGetLastError());
        } else {
            printf("Sent %d bytes\n", result);
        }
    }

    // Cleanup
    closesocket(sock);
    freeaddrinfo(addr_result);
    WSACleanup();

    printf("\nSocket closed successfully\n");
    return 0;
}
