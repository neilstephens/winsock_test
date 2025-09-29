# TCP Connect - Windows Socket Options Explorer

A command-line tool for testing Windows TCP socket options, particularly those related to timeouts and keepalive settings.

WARNING: AI GENERATED

## Important Note on TCP Retries

**Windows does expose `TCP_MAXRT` for per-socket configuration**, but it controls the **maximum retransmission timeout in seconds** (time-based), not the retry count.

Key differences from what the RFC suggests:
- `TCP_MAXRT` sets how long (in seconds) TCP will attempt retransmissions
- It does NOT set the number of retry attempts
- A value of 0 means use system default

Other socket options available for controlling connection reliability:
- **Keepalive settings** (time and interval)
- **Send/receive timeouts**
- **Connection timeout** (implemented via non-blocking connect + select)
- **Linger behavior** on close

## Building

### Option 1: Visual Studio

1. Open Visual Studio Developer Command Prompt
2. Navigate to project directory
3. Run:
```cmd
cmake -B build -S . -G "Visual Studio 17 2022"
cmake --build build --config Release
```

The executable will be in `build\bin\Release\tcp_connect.exe`

### Option 2: CMake with MinGW

```cmd
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build
```

### Option 3: GitHub Actions

Just push to GitHub - the workflow will automatically build both x86 and x64 versions and upload them as artifacts.

## Usage

```
tcp_connect <host> <port> [options]
```

### Examples

Basic connection:
```cmd
tcp_connect.exe google.com 80
```

With keepalive enabled:
```cmd
tcp_connect.exe google.com 80 --keepalive 1 --keepalive-time 30000 --keepalive-interval 5000
```

With connection timeout:
```cmd
tcp_connect.exe 192.168.1.100 8080 --connect-timeout 5000
```

Disable Nagle's algorithm:
```cmd
tcp_connect.exe example.com 443 --nodelay 1
```

Multiple options:
```cmd
tcp_connect.exe api.example.com 443 --keepalive 1 --keepalive-time 60000 --nodelay 1 --send-timeout 10000 --recv-timeout 10000
```

## Available Options

| Option | Description | Socket Option Used |
|--------|-------------|-------------------|
| `--keepalive <0\|1>` | Enable/disable TCP keepalive | `SO_KEEPALIVE` |
| `--keepalive-time <ms>` | Time before first keepalive probe | `SIO_KEEPALIVE_VALS` |
| `--keepalive-interval <ms>` | Interval between probes | `SIO_KEEPALIVE_VALS` |
| `--nodelay <0\|1>` | Disable/enable Nagle's algorithm | `TCP_NODELAY` |
| `--maxseg <bytes>` | Maximum segment size | `TCP_MAXSEG` |
| `--connect-timeout <ms>` | Connection timeout | Non-blocking + `select()` |
| `--send-timeout <ms>` | Send operation timeout | `SO_SNDTIMEO` |
| `--recv-timeout <ms>` | Receive operation timeout | `SO_RCVTIMEO` |
| `--sendbuf <bytes>` | Send buffer size | `SO_SNDBUF` |
| `--recvbuf <bytes>` | Receive buffer size | `SO_RCVBUF` |
| `--linger <sec>` | Linger time on close (-1 to disable) | `SO_LINGER` |
| `--maxrt <sec>` | Maximum retransmission timeout | `TCP_MAXRT` |

## What This Tool Demonstrates

✅ **Real Windows socket options** - every option used in this code is documented and functional

✅ **Proper error handling** - shows which options succeed/fail on your system

✅ **Actual values** - reads back the socket options after connection to show what was actually set

❌ **NOT available on Windows per-socket:**
- TCP retry count (no equivalent to setting number of retransmission attempts)
- TCP user timeout in the Linux sense (though `TCP_MAXRT` provides time-based control)

## Project Structure

```
tcp_connect/
├── tcp_connect.cpp           # Main source file
├── CMakeLists.txt            # CMake configuration
├── .github/
│   └── workflows/
│       └── build.yml         # GitHub Actions workflow
└── README.md                 # This file
```

## References

- [Winsock Socket Options](https://docs.microsoft.com/en-us/windows/win32/winsock/socket-options)
- [WSAIoctl SIO_KEEPALIVE_VALS](https://docs.microsoft.com/en-us/windows/win32/winsock/sio-keepalive-vals)
- [TCP/IP Socket Options](https://docs.microsoft.com/en-us/windows/win32/winsock/ipproto-tcp-socket-options)

## License

Public domain / CC0 - use freely for any purpose.
