#include <iostream>
#include <csignal>
#include <atomic>
#include <cstring>
#include "WebServer.hpp"

// Global pointer to server for signal handler access
std::atomic<Webserver::WebServer*> g_server(nullptr);

void signalHandler(int signum) {
    if (signum == SIGTSTP) {
        std::cout << "\nReceived Ctrl+Z signal. Initiating graceful shutdown...\n";
        Webserver::WebServer* server = g_server.load();
        if (server != nullptr) {
            server->stop();
        }
        // Re-raise signal as SIGINT to exit the program
        std::raise(SIGINT);
    }
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --http              Run in HTTP mode (default)\n"
              << "  --https             Run in HTTPS mode (requires --cert and --key)\n"
              << "  --cert <file>       Path to SSL certificate file (required for HTTPS)\n"
              << "  --key <file>        Path to SSL private key file (required for HTTPS)\n"
              << "  --port <number>     Port number (default: 8080 for HTTP, 8443 for HTTPS)\n"
              << "  --threads <number>  Number of worker threads (default: 4)\n"
              << "  --help              Show this help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " --http --port 8080\n"
              << "  " << program_name << " --https --cert certs/cert.pem --key certs/key.pem --port 8443\n";
}

int main(int argc, char* argv[]) {
    // Default configuration
    bool enable_ssl = false;
    const char* cert_file = nullptr;
    const char* key_file = nullptr;
    uint16_t port = 8080;
    int thread_count = 4;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--http") == 0) {
            enable_ssl = false;
        } else if (strcmp(argv[i], "--https") == 0) {
            enable_ssl = true;
            if (port == 8080) port = 8443;  // Change default port for HTTPS
        } else if (strcmp(argv[i], "--cert") == 0) {
            if (i + 1 < argc) {
                cert_file = argv[++i];
            } else {
                std::cerr << "Error: --cert requires a file path\n";
                return 1;
            }
        } else if (strcmp(argv[i], "--key") == 0) {
            if (i + 1 < argc) {
                key_file = argv[++i];
            } else {
                std::cerr << "Error: --key requires a file path\n";
                return 1;
            }
        } else if (strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port = std::atoi(argv[++i]);
                if (port == 0) {
                    std::cerr << "Error: Invalid port number\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: --port requires a number\n";
                return 1;
            }
        } else if (strcmp(argv[i], "--threads") == 0) {
            if (i + 1 < argc) {
                thread_count = std::atoi(argv[++i]);
                if (thread_count <= 0) {
                    std::cerr << "Error: Invalid thread count\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: --threads requires a number\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option '" << argv[i] << "'\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    // Validate SSL configuration
    if (enable_ssl) {
        if (!cert_file || !key_file) {
            std::cerr << "Error: HTTPS mode requires --cert and --key options\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    // Create server with specified configuration
    std::cout << "Starting web server...\n";
    std::cout << "Mode: " << (enable_ssl ? "HTTPS" : "HTTP") << "\n";
    std::cout << "Port: " << port << "\n";
    std::cout << "Worker threads: " << thread_count << "\n";
    if (enable_ssl) {
        std::cout << "Certificate: " << cert_file << "\n";
        std::cout << "Private key: " << key_file << "\n";
    }
    std::cout << "\n";

    Webserver::WebServer server(thread_count, enable_ssl, cert_file, key_file, port);

    // Store server pointer for signal handler
    g_server.store(&server);

    // Register signal handler for SIGTSTP (Ctrl+Z)
    std::signal(SIGTSTP, signalHandler);

    std::cout << "Web server started. Press Ctrl+Z for graceful shutdown.\n";

    server.acceptConnections();

    // Clean up
    g_server.store(nullptr);

    return 0;
}