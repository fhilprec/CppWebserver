#include "Socket.hpp"
#include <memory>
#include "ThreadSafeQueue.hpp"
#include <thread>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <unistd.h>


namespace Webserver {
class WebServer {
public:

    WebServer(int poolsize) {
        // Do initialization
        ws = std::make_unique<Socket::WebSocket>();
        this->poolsize = poolsize;

        for(int i = 0; i < poolsize; i++){
            // Use lambda to capture 'this' and call member function
            // Use emplace_back to construct thread in-place (threads are non-copyable)
            this->threads.emplace_back([this]() { workerThreadJob(); });
        }

    }
    void acceptConnections() {
        int client_fd;
        while(true) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            client_fd = accept(ws->getSocketFD(), (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                throw std::runtime_error("Failed to accept connection");
                close(ws->getSocketFD());
            }
            else {
                connectionQueue.push(client_fd);
            }
        }
    }

    void workerThreadJob(){
        while(true){
            int client_fd = connectionQueue.pop();
            if (client_fd == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); //improve waiting strategy
                continue; // Queue is empty, wait and try again
            }
            std::ifstream t("files/myHomePage.html");
            std::stringstream buffer;
            buffer << t.rdbuf();
            const std::string response = "HTTP/1.1 200 OK\r\n\r\n " + buffer.str();
            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);
        }
        
    }

    ~WebServer() {
        // No need to manually delete - unique_ptr handles cleanup automatically
    }
private:
    std::unique_ptr<Socket::WebSocket> ws;  // Starts as nullptr, initialized in constructor
    std::vector<std::thread> threads;
    ThreadSafeQueue connectionQueue;
    int poolsize;
};
}