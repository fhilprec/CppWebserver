#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace Socket {
class WebSocket {
public:
    WebSocket(int domain = AF_INET, int type=SOCK_STREAM, int protocol = 0, int level = SOL_SOCKET, int option_name = SO_REUSEADDR, int sin_family = AF_INET, in_addr_t sin_addr = INADDR_ANY, uint16_t sin_port = 8080, int backlog = 3) {
        std::cout << domain << " " << type << " " << protocol << " " << level << " " << option_name << " " << sin_family << " " << sin_addr << " " << sin_port << " " << backlog << "\n";
        server_socket_fd = socket(domain, type, protocol);
        if (server_socket_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }  

        int opt = 1;
        setsockopt(server_socket_fd, level, option_name, &opt, sizeof(opt));

        struct sockaddr_in address;
        address.sin_family = sin_family;
        address.sin_addr.s_addr = sin_addr; // Alle Interfaces
        address.sin_port = htons(sin_port);        // Port 8080

        if (bind(server_socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(server_socket_fd);
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(server_socket_fd, backlog) < 0) {
            close(server_socket_fd); //TODO write class to satisfy RAII
            throw std::runtime_error("Failed to listen on socket");
        }

        std::cout << "Server listening on port " << sin_port << "...\n";

    };





    ~WebSocket() {
        close(server_socket_fd);
    };
    int getSocketFD() const {
        return server_socket_fd;
    }
private:
    int server_socket_fd;
};
}
