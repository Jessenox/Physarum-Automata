#include "CYdLidar.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using namespace std::chrono_literals;

using namespace ydlidar;
using namespace std;

// Global variables
atomic<bool> is_running(true);

// WebSocket session to handle the connection and send data
void websocket_session(tcp::socket socket) {
    try {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();

        CYdLidar laser;
        std::string port;
        std::map<std::string, std::string> ports = ydlidar::lidarPortList();

        if (ports.size() == 1) {
            port = ports.begin()->second;
        } else {
            cout << "Select Lidar port: ";
            cin >> port;
        }

        int baudrate = 115200;
        laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
        laser.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));

        laser.initialize();
        laser.turnOn();

        // Send LiDAR data through WebSocket
        while (is_running) {
            LaserScan scan;
            if (laser.doProcessSimple(scan)) {
                std::stringstream lidar_data;
                for (const auto& point : scan.points) {
                    lidar_data << "x: " << point.range * cos(point.angle)
                               << ", y: " << point.range * sin(point.angle) << "\n";
                }
                ws.write(net::buffer(lidar_data.str()));
            }
            std::this_thread::sleep_for(1s);  // Adjust the send frequency
        }

        laser.turnOff();
        laser.disconnecting();
    } catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed) {
            cerr << "Error: " << se.what() << endl;
        }
    }
}

// WebSocket server function
void server(boost::asio::io_context& ioc, tcp::endpoint endpoint) {
    tcp::acceptor acceptor(ioc, endpoint);

    while (is_running) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);
        std::thread(&websocket_session, std::move(socket)).detach();
    }
}

int main() {
    try {
        net::io_context ioc;

        // WebSocket server listens on port 8080
        tcp::endpoint endpoint(tcp::v4(), 8080);

        // Start the server
        std::thread server_thread([&ioc, endpoint]() {
            server(ioc, endpoint);
        });

        cout << "WebSocket server is running on ws://localhost:8080\n";
        server_thread.join();

    } catch (const std::exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;
}
