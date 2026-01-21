# C++ Web Server with SSL/TLS Support

A multi-threaded HTTP/HTTPS web server implementation in C++ with OpenSSL integration.

## Features

- Multi-threaded request handling with thread pool
- Support for both HTTP and HTTPS (SSL/TLS)
- Graceful shutdown mechanism
- Thread-safe connection queue
- HTTP request parsing

## Architecture

- **WebSocket**: Socket wrapper that handles TCP socket creation and SSL context initialization
- **WebServer**: Main server class with thread pool for handling concurrent connections
- **HttpRequestParser**: Parses incoming HTTP requests
- **ThreadSafeQueue**: Thread-safe queue for distributing client connections to worker threads

## Dependencies

- OpenSSL (libssl-dev)
- C++11 or later
- POSIX threads (pthread)

## Installation

### 1. Install OpenSSL Development Libraries

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# Fedora/RHEL
sudo dnf install openssl-devel

# macOS
brew install openssl
```

### 2. Generate SSL Certificates (for HTTPS)

For development/testing, generate self-signed certificates:

```bash
./generate_certs.sh
```

This creates:
- `certs/cert.pem` - SSL certificate
- `certs/key.pem` - Private key

**Note**: Self-signed certificates will cause browser warnings. For production, use certificates from a trusted Certificate Authority (CA).

### 3. Build the Server

```bash
# Compile with SSL support
g++ -std=c++11 src/main.cpp -o webserver -lssl -lcrypto -pthread

# Or without optimization flags for debugging
g++ -std=c++11 -g src/main.cpp -o webserver -lssl -lcrypto -pthread
```

## Usage

### HTTP Server (port 8080)

```cpp
#include "WebServer.hpp"

int main() {
    // Create server with 4 worker threads, HTTP mode (SSL disabled)
    Webserver::WebServer server(4);
    server.acceptConnections();
    return 0;
}
```

### HTTPS Server (port 443)

```cpp
#include "WebServer.hpp"

int main() {
    // Create HTTPS server with SSL enabled
    // Note: Constructor parameters for WebSocket include SSL settings
    Webserver::WebServer server(4);  // 4 worker threads

    // The WebSocket is created with SSL parameters:
    // WebSocket(domain, type, protocol, level, option_name,
    //           sin_family, sin_addr, sin_port, backlog,
    //           enable_ssl, cert_file, key_file)

    server.acceptConnections();
    return 0;
}
```

### Custom Configuration Example

To enable SSL, you need to modify the WebServer constructor to pass SSL parameters to the WebSocket:

```cpp
// In WebServer.hpp constructor, replace:
ws = std::make_unique<Socket::WebSocket>();

// With (for HTTPS on port 443):
ws = std::make_unique<Socket::WebSocket>(
    AF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_REUSEADDR,
    AF_INET, INADDR_ANY, 443, 3,
    true, "certs/cert.pem", "certs/key.pem"
);
```

## SSL/TLS Configuration

### WebSocket Constructor Parameters

```cpp
WebSocket(
    int domain = AF_INET,              // IPv4
    int type = SOCK_STREAM,            // TCP
    int protocol = 0,                  // Default protocol
    int level = SOL_SOCKET,            // Socket level options
    int option_name = SO_REUSEADDR,    // Allow address reuse
    int sin_family = AF_INET,          // IPv4
    in_addr_t sin_addr = INADDR_ANY,   // Bind to all interfaces
    uint16_t sin_port = 8080,          // Port number
    int backlog = 3,                   // Connection queue size
    bool enable_ssl = false,           // Enable SSL/TLS
    const char* cert_file = nullptr,   // Path to certificate
    const char* key_file = nullptr     // Path to private key
)
```

### SSL Implementation Details

The SSL integration uses OpenSSL and provides:

1. **SSL Context Initialization**: Creates and configures SSL_CTX with TLS_server_method()
2. **Certificate Loading**: Loads X.509 certificate and private key in PEM format
3. **Key Verification**: Validates that the private key matches the certificate
4. **Per-Connection SSL**: Each client connection gets its own SSL structure
5. **SSL Handshake**: Performs TLS handshake with SSL_accept()
6. **Encrypted I/O**: Uses SSL_read() and SSL_write() for encrypted communication
7. **Graceful Cleanup**: Properly shuts down SSL connections with SSL_shutdown()

## Testing

### Test HTTP Server
```bash
curl http://localhost:8080/
```

### Test HTTPS Server (with self-signed cert)
```bash
# -k flag ignores certificate warnings
curl -k https://localhost:443/

# Or use a browser (will show security warning)
# Click "Advanced" -> "Proceed to localhost"
```

## Graceful Shutdown

Press `Ctrl+Z` to trigger graceful shutdown. The server will:
1. Stop accepting new connections
2. Wait for worker threads to complete current requests
3. Clean up resources and exit

## Security Notes

- Self-signed certificates are for **development only**
- For production:
  - Use certificates from trusted CA (Let's Encrypt, etc.)
  - Run HTTPS on port 443 (requires root/sudo)
  - Implement proper error handling for SSL failures
  - Consider adding support for TLS 1.3 minimum version
  - Implement proper cipher suite selection

## Project Structure

```
CppWebserver/
├── src/
│   ├── Socket.hpp           # Socket and SSL context management
│   ├── WebServer.hpp        # Main server implementation
│   ├── HttpRequestParser.hpp  # HTTP request parsing
│   ├── ThreadSafeQueue.hpp  # Thread-safe queue
│   └── main.cpp             # Entry point
├── certs/                   # SSL certificates (generated)
│   ├── cert.pem
│   └── key.pem
├── files/                   # Static files to serve
│   └── myHomePage.html
├── generate_certs.sh        # Certificate generation script
└── README.md
```

## Future Improvements

- HTTP/2 support
- Certificate chain validation
- OCSP stapling
- Perfect forward secrecy
- Connection keep-alive
- Request routing system
- Static file serving improvements