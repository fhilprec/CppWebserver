
#include <memory>
#include <thread>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <unistd.h>
#include <condition_variable>

#include "ThreadSafeQueue.hpp"
#include "Socket.hpp"
#include "HttpRequestParser.hpp"


namespace Webserver {
class WebServer {
public:

    WebServer(int poolsize) {
        // Do initialization
        ws = std::make_unique<Socket::WebSocket>();
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
        while(true) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            client_fd = accept(ws->getSocketFD(), (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                throw std::runtime_error("Failed to accept connection");
                close(ws->getSocketFD());
            }
            else {
                std::lock_guard<std::mutex> lock(cv_mtx);
                connectionQueue.push(client_fd);
                cv.notify_one();
            }
        }
    }

    void workerThreadJob(){
        while(true){
            std::unique_lock<std::mutex> lock(cv_mtx);
            cv.wait(lock, [this]{ return !connectionQueue.isEmpty() || shutdown; });
            //here multple threads can wake up -> spurious wakeup
            //that does not matter though since pop() is thread safe and we check for client_fd == -1

            int client_fd = connectionQueue.pop();
            lock.unlock();
            if (client_fd == -1) {
                continue; // Queue is empty, wait and try again
            }

            // Optionally Read the HTTP request
            // char readbuffer[4096] = {0};
            // recv(client_fd, readbuffer, sizeof(readbuffer), 0);
            // std::cout << "=== Request on fd " << client_fd << " ===\n";
            // std::cout << readbuffer << "\n";
            // std::cout << "=== End Request ===\n";


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

    //condition_variable block
    std::condition_variable cv;
    std::mutex cv_mtx;
    bool shutdown;
};
} // namespace Webserver