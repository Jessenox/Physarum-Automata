#include "CYdLidar.h"
#include <string>
#include <map>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>
#include <libfreenect.hpp>
#include <thread>
#include <atomic>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <sstream>  // For std::ostringstream

using namespace std;
using namespace ydlidar;
using namespace cv;

// Socket path
const char* socket_path = "/tmp/unixSocket8";

// Function to send data to the server
void sendDataToServer(int sock, const std::string &data) {
    if (send(sock, data.c_str(), data.length(), 0) < 0) {
        std::cerr << "Failed to send data. Error: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Data sent to server: " << data << std::endl;
    }
}

// Freenect variables
freenect_context *f_ctx;
freenect_device *f_dev;
Mat depthMat(Size(640, 480), CV_16UC1);
Mat rgbMat(Size(640, 480), CV_8UC4, Scalar(0, 0, 0, 0)); // Initialize with transparent
atomic<bool> is_running(true);

// Function to format the LiDAR data as a JSON-like string
std::string formatLidarDataManually(const LaserScan& scan) {
    std::ostringstream ss;
    ss << "{\"points\":[";  // Start of JSON object

    for (size_t i = 0; i < scan.points.size(); ++i) {
        float x = scan.points[i].range * cos(scan.points[i].angle);
        float y = scan.points[i].range * sin(scan.points[i].angle);
        ss << "{\"x\":" << x << ",\"y\":" << y << "}";

        // Add a comma if it's not the last element
        if (i < scan.points.size() - 1) {
            ss << ",";
        }
    }

    ss << "]}";  // End of JSON object
    return ss.str();
}


void depth_cb(freenect_device *dev, void *depth, uint32_t timestamp) {
    Mat depth_tmp(Size(640, 480), CV_16UC1, depth);
    depth_tmp.copyTo(depthMat);
    cout << "Depth frame received at timestamp: " << timestamp << endl;
}

void rgb_cb(freenect_device *dev, void *rgb, uint32_t timestamp) {
    Mat rgb_tmp(Size(640, 480), CV_8UC3, rgb);
    cvtColor(rgb_tmp, rgbMat, COLOR_RGB2RGBA);
    cout << "RGB frame received at timestamp: " << timestamp << endl;
}

void freenect_thread_func() {
    while (is_running) {
        freenect_process_events(f_ctx);
    }
}

int main(int argc, char *argv[]) {
    // Initialize the socket connection to the server
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket. Error: " << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to the socket. Error: " << strerror(errno) << std::endl;
        close(sock);
        return 1;
    }
    std::cout << "Connected to server at " << socket_path << std::endl;

    std::string port;
    ydlidar::os_init();

    // Initialize Freenect
    if (freenect_init(&f_ctx, NULL) < 0) {
        cerr << "Freenect init failed" << endl;
        close(sock);
        return -1;
    }
    freenect_set_log_level(f_ctx, FREENECT_LOG_DEBUG);
    int nr_devices = freenect_num_devices(f_ctx);
    if (nr_devices < 1) {
        cerr << "No Kinect devices found" << endl;
        close(sock);
        return -1;
    }
    if (freenect_open_device(f_ctx, &f_dev, 0) < 0) {
        cerr << "Could not open Kinect device" << endl;
        close(sock);
        return -1;
    }

    // Set Kinect callbacks
    freenect_set_depth_callback(f_dev, depth_cb);
    freenect_set_video_callback(f_dev, rgb_cb);
    freenect_start_depth(f_dev);
    freenect_start_video(f_dev);
    std::thread freenect_thread(freenect_thread_func);

    // Get available LiDAR ports
    std::map<std::string, std::string> ports = ydlidar::lidarPortList();
    if (ports.size() == 1) {
        port = ports.begin()->second;
    } else {
        int id = 0;
        for (auto it = ports.begin(); it != ports.end(); it++) {
            printf("%d. %s\n", id, it->first.c_str());
            id++;
        }
        if (ports.empty()) {
            printf("No LiDAR was detected. Please enter the LiDAR serial port:");
            std::cin >> port;
        } else {
            while (ydlidar::os_isOk()) {
                printf("Please select the LiDAR port:");
                std::string number;
                std::cin >> number;
                if ((size_t)atoi(number.c_str()) >= ports.size()) continue;
                auto it = ports.begin();
                id = atoi(number.c_str());
                while (id) {
                    id--;
                    it++;
                }
                port = it->second;
                break;
            }
        }
    }

    // Baud rate selection
    int baudrate = 115200;
    bool isSingleChannel = true;
    float frequency = 8.0;

    CYdLidar laser;
    laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
    laser.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));
    laser.setlidaropt(LidarPropSingleChannel, &isSingleChannel, sizeof(bool));
    laser.setlidaropt(LidarPropScanFrequency, &frequency, sizeof(float));

    // Initialize LiDAR
    bool ret = laser.initialize();
    if (ret) {
        ret = laser.turnOn();
    } else {
        cerr << "Error initializing YDLIDAR: " << laser.DescribeError() << endl;
        close(sock);
        return -1;
    }

    // Main loop for processing and sending LiDAR data
    while (ret && ydlidar::os_isOk()) {
        LaserScan scan;

        if (laser.doProcessSimple(scan)) {
            // Collect LiDAR data
            std::string lidarData = formatLidarDataManually(scan);
            /*
            for (const auto& point : scan.points) {
                float x = point.range * cos(point.angle);
                float y = point.range * sin(point.angle);
                lidarData += "X: " + std::to_string(x) + ", Y: " + std::to_string(y) + "\n";
            }*/

            // Send the LiDAR data to the server
            if (!lidarData.empty()) {
                sendDataToServer(sock, lidarData);
            }
        } else {
            std::cerr << "Failed to get LiDAR data" << std::endl;
        }

        // Small delay to avoid flooding the server
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Clean up
    close(sock);  // Close the socket connection
    laser.turnOff();
    laser.disconnecting();
    is_running = false;
    freenect_thread.join();
    freenect_stop_depth(f_dev);
    freenect_stop_video(f_dev);
    freenect_close_device(f_dev);
    freenect_shutdown(f_ctx);

    return 0;
}
