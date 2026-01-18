#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

// Einfacher TCP Server
void simple_server() {
    // 1. Socket erstellen
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return;
    }
    
    // 2. Socket konfigurieren
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 3. Adresse binden
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Alle Interfaces
    address.sin_port = htons(8080);        // Port 8080
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return;
    }
    
    // 4. Lauschen
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        return;
    }
    
    std::cout << "Server lauscht auf Port 8080...\n";
    
    // 5. Verbindung akzeptieren
    int client_fd;
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        return;
    }
    
    std::cout << "Client verbunden!\n";
    
    // 6. Daten empfangen
    char buffer[1024] = {0};
    read(client_fd, buffer, 1024);
    std::cout << "Empfangen: " << buffer << "\n";
    
    // 7. Daten senden
    const char* response = "HTTP/1.1 200 OK\r\n\r\nHello World!";
    send(client_fd, response, strlen(response), 0);
    
    // 8. Aufräumen
    close(client_fd);
    close(server_fd);
}

// Einfacher TCP Client
void simple_client() {
    // 1. Socket erstellen
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        return;
    }
    
    // 2. Server-Adresse
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    
    // IP-Adresse konvertieren
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return;
    }
    
    // 3. Verbinden
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }
    
    std::cout << "Verbunden mit Server!\n";
    
    // 4. Daten senden
    const char* message = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(sock, message, strlen(message), 0);
    
    // 5. Antwort empfangen
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    std::cout << "Antwort: " << buffer << "\n";
    
    // 6. Aufräumen
    close(sock);
}

int main() {
    // Test in zwei Terminals:
    // Terminal 1: simple_server();
    // Terminal 2: simple_client();
    
    simple_client();
    return 0;
}