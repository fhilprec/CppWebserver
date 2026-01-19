#include <iostream>
#include "WebServer.hpp"

int main() {
    Webserver::WebServer server(1);
    server.acceptConnections(); //this is a waiting call no closing connections yet implemented
    return 0;
}