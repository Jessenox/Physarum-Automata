#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int main() {
    const char* socket_path = "/tmp/unixSocket9";

    // Create the UNIX socket
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // Set up the socket address structure
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // Connect to the UNIX socket server
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to the socket." << std::endl;
        close(sock);
        return 1;
    }

    char buffer[256];

    // Main loop to listen for commands
    while (true) {
        std::cout << "Waiting for command..." << std::endl;
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string command(buffer);
            std::cout << "Received command: " << command << std::endl;

            // Process the command ('w', 's', 'a', 'd')
            if (command == "w") {
                std::cout << "Move forward" << std::endl;
            } else if (command == "s") {
                std::cout << "Move backward" << std::endl;
            } else if (command == "a") {
                std::cout << "Turn left" << std::endl;
            } else if (command == "d") {
                std::cout << "Turn right" << std::endl;
            }
        }
    }

    close(sock);
    return 0;
}
