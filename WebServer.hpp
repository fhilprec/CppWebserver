#include "Socket.hpp"
#include <memory>

namespace Webserver {
    class WebServer {
    public:
        WebServer() {
            // Do initialization
            ws = std::make_unique<Socket::WebSocket>();
        }
        ~WebServer() {
            // No need to manually delete - unique_ptr handles cleanup automatically
        }
    private:
        std::unique_ptr<Socket::WebSocket> ws;  // Starts as nullptr, initialized in constructor
    };
}