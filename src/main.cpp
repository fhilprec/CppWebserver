#include <iostream>
#include <csignal>
#include <atomic>
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

int main() {
    Webserver::WebServer server(1);

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