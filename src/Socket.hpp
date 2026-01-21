#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace Socket {
class WebSocket {
public:
    WebSocket(int domain = AF_INET, int type=SOCK_STREAM, int protocol = 0, int level = SOL_SOCKET, int option_name = SO_REUSEADDR, int sin_family = AF_INET, in_addr_t sin_addr = INADDR_ANY, uint16_t sin_port = 8080, int backlog = 3, bool enable_ssl = false, const char* cert_file = nullptr, const char* key_file = nullptr) {
        // std::cout << domain << " " << type << " " << protocol << " " << level << " " << option_name << " " << sin_family << " " << sin_addr << " " << sin_port << " " << backlog << "\n";
        server_socket_fd = socket(domain, type, protocol);
        if (server_socket_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }  

        int opt = 1;
        setsockopt(server_socket_fd, level, option_name, &opt, sizeof(opt));

        struct sockaddr_in address;
        address.sin_family = sin_family;
        address.sin_addr.s_addr = sin_addr; //All Intefaces
        address.sin_port = htons(sin_port);        // Port 8080

        if (bind(server_socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(server_socket_fd);
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(server_socket_fd, backlog) < 0) {
            close(server_socket_fd); //TODO write class to satisfy RAII
            throw std::runtime_error("Failed to listen on socket");
        }

        // Initialize SSL if enabled
        this->ssl_enabled = enable_ssl;
        this->ssl_ctx = nullptr;

        if (ssl_enabled) {
            // Initialize OpenSSL library
            SSL_load_error_strings();
            OpenSSL_add_ssl_algorithms();

            // Create SSL context
            const SSL_METHOD* method = TLS_server_method();
            ssl_ctx = SSL_CTX_new(method);
            if (!ssl_ctx) {
                close(server_socket_fd);
                throw std::runtime_error("Failed to create SSL context");
            }

            // Load certificate and private key
            if (!cert_file || !key_file) {
                SSL_CTX_free(ssl_ctx);
                close(server_socket_fd);
                throw std::runtime_error("SSL enabled but certificate or key file not provided");
            }

            if (SSL_CTX_use_certificate_file(ssl_ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_CTX_free(ssl_ctx);
                close(server_socket_fd);
                throw std::runtime_error("Failed to load certificate file");
            }

            if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_CTX_free(ssl_ctx);
                close(server_socket_fd);
                throw std::runtime_error("Failed to load private key file");
            }

            // Verify that the private key matches the certificate
            if (!SSL_CTX_check_private_key(ssl_ctx)) {
                SSL_CTX_free(ssl_ctx);
                close(server_socket_fd);
                throw std::runtime_error("Private key does not match certificate");
            }

            std::cout << "SSL/TLS enabled - Server listening on port " << sin_port << " (HTTPS)...\n";
        } else {
            std::cout << "Server listening on port " << sin_port << " (HTTP)...\n";
        }

    };





    ~WebSocket() {
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
        }
        close(server_socket_fd);
    };

    int getSocketFD() const {
        return server_socket_fd;
    }

    SSL_CTX* getSSLContext() const {
        return ssl_ctx;
    }

    bool isSSLEnabled() const {
        return ssl_enabled;
    }

private:
    int server_socket_fd;
    SSL_CTX* ssl_ctx;
    bool ssl_enabled;
};
}
