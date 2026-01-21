
#include <memory>
#include <thread>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <condition_variable>

#include "ThreadSafeQueue.hpp"
#include "Socket.hpp"
#include "HttpRequestParser.hpp"


namespace Webserver {
class WebServer {
public:

    WebServer(int poolsize, bool enable_ssl = false,
              const char* cert_file = nullptr, const char* key_file = nullptr,
              uint16_t port = 8080) {
        // Do initialization
        if (enable_ssl) {
            ws = std::make_unique<Socket::WebSocket>(
                AF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_REUSEADDR,
                AF_INET, INADDR_ANY, port, 3,
                true, cert_file, key_file
            );
        } else {
            ws = std::make_unique<Socket::WebSocket>(
                AF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_REUSEADDR,
                AF_INET, INADDR_ANY, port, 3,
                false, nullptr, nullptr
            );
        }

        this->poolsize = poolsize;
        this->shutdown = false;

        for(int i = 0; i < poolsize; i++){
            // Use lambda to capture 'this' and call member function
            // Use emplace_back to construct thread in-place (threads are non-copyable)
            this->threads.emplace_back([this]() { workerThreadJob(); });
        }

    }
    void acceptConnections() {
        int client_fd;
        while(!shutdown) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            client_fd = accept(ws->getSocketFD(), (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (!shutdown) {  // Only throw if not shutting down
                    std::cerr << "Failed to accept connection\n";
                }
                break;
            }
            else {
                std::lock_guard<std::mutex> lock(cv_mtx);
                connectionQueue.push(client_fd);
                cv.notify_one();
            }
        }
    }

    // Helper struct to hold both socket FD and SSL pointer
    struct ClientConnection {
        int fd;
        SSL* ssl;
        ClientConnection() : fd(-1), ssl(nullptr) {}
        ClientConnection(int f, SSL* s) : fd(f), ssl(s) {}
    };

    void workerThreadJob(){
        while(true){
            std::unique_lock<std::mutex> lock(cv_mtx);
            cv.wait(lock, [this]{ return !connectionQueue.isEmpty() || shutdown; });

            // Check if we should shut down
            if (shutdown && connectionQueue.isEmpty()) {
                break;  // Exit thread gracefully
            }

            //here multple threads can wake up -> spurious wakeup
            //that does not matter though since pop() is thread safe and we check for client_fd == -1

            int client_fd = connectionQueue.pop();
            lock.unlock();
            if (client_fd == -1) {
                continue; // Queue is empty, wait and try again
            }

            SSL* ssl = nullptr;
            bool ssl_enabled = ws->isSSLEnabled();

            // If SSL is enabled, perform SSL handshake
            if (ssl_enabled) {
                ssl = SSL_new(ws->getSSLContext());
                if (!ssl) {
                    std::cerr << "Failed to create SSL structure for client fd " << client_fd << "\n";
                    close(client_fd);
                    continue;
                }

                SSL_set_fd(ssl, client_fd);

                int ssl_accept_result = SSL_accept(ssl);
                if (ssl_accept_result <= 0) {
                    int ssl_error = SSL_get_error(ssl, ssl_accept_result);
                    std::cerr << "SSL handshake failed for client fd " << client_fd
                              << " with error code: " << ssl_error << "\n";
                    ERR_print_errors_fp(stderr);
                    SSL_free(ssl);
                    close(client_fd);
                    continue;
                }
            }

            // Read HTTP request with error handling
            char readbuffer[4096] = {0};
            ssize_t bytes_received;

            if (ssl_enabled) {
                bytes_received = SSL_read(ssl, readbuffer, sizeof(readbuffer) - 1);
            } else {
                bytes_received = recv(client_fd, readbuffer, sizeof(readbuffer) - 1, 0);
            }

            std::cout << "SSL enabled: " << (ssl_enabled ? "Yes" : "No") << "\n";

            std::cout << readbuffer << std::endl;

            if (bytes_received < 0) {
                std::cerr << "Error receiving data from client fd " << client_fd << "\n";
                if (ssl) SSL_free(ssl);
                close(client_fd);
                continue;
            } else if (bytes_received == 0) {
                std::cerr << "Client fd " << client_fd << " closed connection\n";
                if (ssl) SSL_free(ssl);
                close(client_fd);
                continue;
            }

            // Null-terminate the buffer (we reserved space for this)
            readbuffer[bytes_received] = '\0';

            std::string response = requestHandler(std::string(readbuffer));

            // Send response
            if (ssl_enabled) {
                SSL_write(ssl, response.c_str(), response.size());
            } else {
                send(client_fd, response.c_str(), response.size(), 0);
            }

            // Clean up SSL connection
            if (ssl) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
            }
            close(client_fd);
        }

    }

    std::string requestHandler(const std::string& rawRequest) {
        HttpRequestParser parser;
        parser.parseRequest(rawRequest);

        std::string method = parser.getMethod();
        std::string path = parser.getPath();
        std::string version = parser.getVersion();
        auto headers = parser.getHeaders();
        std::string body = parser.getBody();

        std::string response;

        switch(method[0]) {
            case 'G': //GET
                std::cout << "Received GET request for path: " << path << "\n";
                if (path == "/") {
                    std::ifstream t("files/myHomePage.html");
                    std::stringstream buffer;
                    buffer << t.rdbuf();
                    response = "HTTP/1.1 200 OK\r\n\r\n " + buffer.str();
                }
                else {
                    response = "HTTP/1.1 404 Not Found\r\n\r\n <h1>The page you are looking for does not exist</h1>";
                }
                
                break;
            case 'P': //POST
                std::cout << "Received POST request for path: " << path << "\n";
                response = "HTTP/1.1 200 OK\r\n\r\n <h1>POST request received</h1>";
                break;
            default:
                response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n <h1>405 Method Not Allowed</h1>";
                break;
        }

        return response;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(cv_mtx);
            shutdown = true;
        }
        // Notify all waiting threads to wake up and check shutdown flag
        cv.notify_all();

        // Wait for all worker threads to finish
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    ~WebServer() {
        // Ensure threads are stopped before destruction
        if (!shutdown) {
            stop();
        }
    }
    
private:
    std::unique_ptr<Socket::WebSocket> ws;  // Starts as nullptr, initialized in constructor
    std::vector<std::thread> threads;
    ThreadSafeQueue connectionQueue;
    int poolsize;

    //condition_variable block
    std::condition_variable cv;
    std::mutex cv_mtx;
    bool shutdown;

};
} // namespace Webserver