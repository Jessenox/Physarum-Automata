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

using namespace std;
using namespace ydlidar;
using namespace cv;

#if defined(_MSC_VER)
#pragma comment(lib, "ydlidar_sdk.lib")
#endif

// Freenect variables
freenect_context *f_ctx;
freenect_device *f_dev;
Mat depthMat(Size(640, 480), CV_16UC1);
Mat rgbMat(Size(640, 480), CV_8UC4, Scalar(0, 0, 0, 0)); // Initialize with transparent
atomic<bool> is_running(true);

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

void drawGrid(sf::RenderTarget &target, float gridSpacing, float offsetX, float offsetY) {
    sf::Color gridColor(200, 200, 200); // Light gray color
    for (float x = offsetX; x < target.getSize().x; x += gridSpacing) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x, 0), gridColor),
            sf::Vertex(sf::Vector2f(x, target.getSize().y), gridColor)
        };
        target.draw(line, 2, sf::Lines);
    }
    for (float x = offsetX; x > 0; x -= gridSpacing) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x, 0), gridColor),
            sf::Vertex(sf::Vector2f(x, target.getSize().y), gridColor)
        };
        target.draw(line, 2, sf::Lines);
    }
    for (float y = offsetY; y < target.getSize().y; y += gridSpacing) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(0, y), gridColor),
            sf::Vertex(sf::Vector2f(target.getSize().x, y), gridColor)
        };
        target.draw(line, 2, sf::Lines);
    }
    for (float y = offsetY; y > 0; y -= gridSpacing) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(0, y), gridColor),
            sf::Vertex(sf::Vector2f(target.getSize().x, y), gridColor)
        };
        target.draw(line, 2, sf::Lines);
    }
}

sf::Color getPointColor(float distance, float maxRange) {
    float ratio = distance / maxRange;
    return sf::Color(255 * (1 - ratio), 255 * ratio, 0); // Color from red to green
}

void drawNorthIndicator(sf::RenderTexture &lidarTexture, float offsetX, float offsetY) {
    sf::Color northColor(255, 0, 0); // Red color for the north indicator
    float length = 50.0f; // Length of the north indicator line

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(offsetX, offsetY), northColor),
        sf::Vertex(sf::Vector2f(offsetX, offsetY - length), northColor) // Line pointing upwards
    };
    lidarTexture.draw(line, 2, sf::Lines);
}

void detectAndDrawDepthObstacles(Mat &depthMat, sf::RenderTexture &lidarTexture, float offsetX, float offsetY, float scale, vector<Point2f> &depthObstacles) {
    for (int y = 0; y < depthMat.rows; y += 10) {
        for (int x = 0; x < depthMat.cols; x += 10) {
            uint16_t depth_value = depthMat.at<uint16_t>(y, x);
            if (depth_value > 0 && depth_value < 2047) {
                float depth_in_meters = depth_value / 1000.0f;

                float adjustedX = offsetX + (x - depthMat.cols / 2) * scale / depth_in_meters;
                float adjustedY = offsetY - (y - depthMat.rows / 2) * scale / depth_in_meters; // Invert Y to match screen coordinates

                sf::CircleShape circle(3); // Small circle to represent the obstacle
                circle.setPosition(adjustedX, adjustedY);
                circle.setFillColor(sf::Color::Red);

                lidarTexture.draw(circle);
                
                // Add the obstacle to the vector
                depthObstacles.push_back(Point2f(adjustedX, adjustedY));
            }
        }
    }
}

void compareObstacles(const vector<Point2f> &depthObstacles, const vector<Point2f> &lidarObstacles) {
    cout << "Comparing Obstacles:" << endl;
    for (const auto &d : depthObstacles) {
        bool matchFound = false;
        for (const auto &l : lidarObstacles) {
            float distance = sqrt(pow(d.x - l.x, 2) + pow(d.y - l.y, 2));
            if (distance < 10.0f) { // If the distance is less than a threshold, consider it a match
                matchFound = true;
                break;
            }
        }
        if (matchFound) {
            cout << "Match found for depth obstacle at (" << d.x << ", " << d.y << ")" << endl;
        } else {
            cout << "No match for depth obstacle at (" << d.x << ", " << d.y << ")" << endl;
        }
    }
}

void freenect_thread_func() {
    while (is_running) {
        freenect_process_events(f_ctx);
    }
}

void print_frame_modes() {
    cout << "Available video modes:" << endl;
    for (int res = FREENECT_RESOLUTION_LOW; res <= FREENECT_RESOLUTION_HIGH; ++res) {
        for (int fmt = FREENECT_VIDEO_RGB; fmt <= FREENECT_VIDEO_IR_8BIT; ++fmt) {
            freenect_frame_mode mode = freenect_find_video_mode((freenect_resolution)res, (freenect_video_format)fmt);
            if (mode.is_valid) {
                cout << "Resolution: " << res << ", Format: " << fmt << ", Width: " << mode.width << ", Height: " << mode.height << ", Bytes per pixel: " << mode.data_bits_per_pixel << endl;
            }
        }
    }

    cout << "Available depth modes:" << endl;
    for (int res = FREENECT_RESOLUTION_LOW; res <= FREENECT_RESOLUTION_HIGH; ++res) {
        for (int fmt = FREENECT_DEPTH_11BIT; fmt <= FREENECT_DEPTH_REGISTERED; ++fmt) {
            freenect_frame_mode mode = freenect_find_depth_mode((freenect_resolution)res, (freenect_depth_format)fmt);
            if (mode.is_valid) {
                cout << "Resolution: " << res << ", Format: " << fmt << ", Width: " << mode.width << ", Height: " << mode.height << ", Bytes per pixel: " << mode.data_bits_per_pixel << endl;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    std::string port;
    ydlidar::os_init();

    // Initialize Freenect
    if (freenect_init(&f_ctx, NULL) < 0) {
        cerr << "Freenect init failed" << endl;
        return -1;
    }
    freenect_set_log_level(f_ctx, FREENECT_LOG_DEBUG);
    int nr_devices = freenect_num_devices(f_ctx);
    if (nr_devices < 1) {
        cerr << "No Kinect devices found" << endl;
        return -1;
    }
    if (freenect_open_device(f_ctx, &f_dev, 0) < 0) {
        cerr << "Could not open Kinect device" << endl;
        return -1;
    }

    // Print available frame modes
    print_frame_modes();

    // Set the video and depth modes
    freenect_frame_mode video_mode = freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
    if (!video_mode.is_valid) {
        cerr << "Invalid video mode" << endl;
        return -1;
    }
    freenect_frame_mode depth_mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT);
    if (!depth_mode.is_valid) {
        cerr << "Invalid depth mode" << endl;
        return -1;
    }

    if (freenect_set_video_mode(f_dev, video_mode) < 0) {
        cerr << "Failed to set video mode" << endl;
        return -1;
    }

    if (freenect_set_depth_mode(f_dev, depth_mode) < 0) {
        cerr << "Failed to set depth mode" << endl;
        return -1;
    }

    freenect_set_depth_callback(f_dev, depth_cb);
    freenect_set_video_callback(f_dev, rgb_cb);
    freenect_start_depth(f_dev);
    freenect_start_video(f_dev);

    // Start Kinect processing thread
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
    printf("Baudrate: %d\n", baudrate); 
    if (!ydlidar::os_isOk()) {
        return 0;
    }

    // Check for single channel communication
    bool isSingleChannel = false;
    std::string input_channel;
    isSingleChannel = true;

    if (!ydlidar::os_isOk()) {
        return 0;
    }

    // Scan frequency
    float frequency = 8.0;
    while (ydlidar::os_isOk() && !isSingleChannel) {
        printf("Please enter the LiDAR scan frequency[5-12]:");
        std::string input_frequency;
        std::cin >> input_frequency;
        frequency = atof(input_frequency.c_str());
        if (frequency <= 12 && frequency >= 5.0) {
            break;
        }
        fprintf(stderr, "Invalid scan frequency. The scanning frequency range is 5 to 12 Hz. Please re-enter.\n");
    }

    if (!ydlidar::os_isOk()) {
        return 0;
    }

    CYdLidar laser;
    //////////////////////string property/////////////////
    laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
    std::string ignore_array;
    ignore_array.clear();
    laser.setlidaropt(LidarPropIgnoreArray, ignore_array.c_str(), ignore_array.size());

    //////////////////////int property/////////////////
    laser.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));
    int optval = TYPE_TRIANGLE;
    laser.setlidaropt(LidarPropLidarType, &optval, sizeof(int));
    optval = YDLIDAR_TYPE_SERIAL;
    laser.setlidaropt(LidarPropDeviceType, &optval, sizeof(int));
    optval = isSingleChannel ? 3 : 4;
    laser.setlidaropt(LidarPropSampleRate, &optval, sizeof(int));
    optval = 4;
    laser.setlidaropt(LidarPropAbnormalCheckCount, &optval, sizeof(int));

    //////////////////////bool property/////////////////
    bool b_optvalue = false;
    laser.setlidaropt(LidarPropFixedResolution, &b_optvalue, sizeof(bool));
    laser.setlidaropt(LidarPropReversion, &b_optvalue, sizeof(bool));
    laser.setlidaropt(LidarPropInverted, &b_optvalue, sizeof(bool));
    b_optvalue = true;
    laser.setlidaropt(LidarPropAutoReconnect, &b_optvalue, sizeof(bool));
    laser.setlidaropt(LidarPropSingleChannel, &isSingleChannel, sizeof(bool));
    b_optvalue = false;
    laser.setlidaropt(LidarPropIntenstiy, &b_optvalue, sizeof(bool));
    b_optvalue = true;
    laser.setlidaropt(LidarPropSupportMotorDtrCtrl, &b_optvalue, sizeof(bool));
    b_optvalue = false;
    laser.setlidaropt(LidarPropSupportHeartBeat, &b_optvalue, sizeof(bool));

    //////////////////////float property/////////////////
    float f_optvalue = 180.0f;
    laser.setlidaropt(LidarPropMaxAngle, &f_optvalue, sizeof(float));
    f_optvalue = -180.0f;
    laser.setlidaropt(LidarPropMinAngle, &f_optvalue, sizeof(float));
    f_optvalue = 64.0f;
    laser.setlidaropt(LidarPropMaxRange, &f_optvalue, sizeof(float));
    f_optvalue = 0.05f;
    laser.setlidaropt(LidarPropMinRange, &f_optvalue, sizeof(float));
    laser.setlidaropt(LidarPropScanFrequency, &frequency, sizeof(float));

    // Initialize LiDAR
    bool ret = laser.initialize();
    if (ret) {
        // Start scanning
        ret = laser.turnOn();
    } else {
        cerr << "Error initializing YDLIDAR: " << laser.DescribeError() << endl;
        return -1;
    }

    sf::RenderWindow window(sf::VideoMode(1280, 720), "Camera and LiDAR Visualization");

    // Create SFML texture and sprite for the camera feed
    sf::Texture cameraTexture;
    sf::Sprite cameraSprite;

    // Main loop
    while (ret && window.isOpen() && ydlidar::os_isOk()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        LaserScan scan;

        if (laser.doProcessSimple(scan)) {
            window.clear();

            // Update the camera texture with the frame data
            if (!cameraTexture.create(rgbMat.cols, rgbMat.rows)) {
                cerr << "Failed to create texture" << endl;
                break;
            }
            cameraTexture.update(rgbMat.ptr());

            cameraSprite.setTexture(cameraTexture);
            cameraSprite.setScale(
                window.getSize().x / static_cast<float>(cameraTexture.getSize().x),
                window.getSize().y / static_cast<float>(cameraTexture.getSize().y)
            );

            // Draw the camera feed
            window.draw(cameraSprite);

            // Draw the LiDAR minimap
            sf::RenderTexture lidarTexture;
            lidarTexture.create(300, 300);
            lidarTexture.clear(sf::Color::White);

            float gridSpacing = 20.0f; // Grid spacing in pixels
            float offsetX = 150.0f; // Center of the minimap
            float offsetY = 150.0f; // Center of the minimap
            float scale = 50.0f; // Scale for LiDAR points

            drawGrid(lidarTexture, gridSpacing, offsetX, offsetY);
            drawNorthIndicator(lidarTexture, offsetX, offsetY);

            // Detect and draw obstacles from the depth sensor
            vector<Point2f> depthObstacles;
            detectAndDrawDepthObstacles(depthMat, lidarTexture, offsetX, offsetY, scale, depthObstacles);

            // Draw the LiDAR itself (center point)
            sf::CircleShape lidarShape(5); // Larger circle for the center
            lidarShape.setFillColor(sf::Color::Blue); // Blue color for the center
            lidarShape.setPosition(offsetX - lidarShape.getRadius(), offsetY - lidarShape.getRadius());
            lidarTexture.draw(lidarShape);

            // Draw the points from LiDAR
            vector<Point2f> lidarObstacles;
            for (const auto& point : scan.points) {
                // Convert polar coordinates to Cartesian coordinates
                float x = point.range * cos(point.angle);
                float y = point.range * sin(point.angle);

                // Scale and translate points to fit minimap
                float adjustedX = offsetX + x * scale;
                float adjustedY = offsetY - y * scale; // Invert Y to match screen coordinates

                // Set point color based on distance
                sf::Color pointColor = getPointColor(point.range, 64.0f); // Assuming max range is 64 meters

                sf::CircleShape circle(2); // Small circle to represent the point
                circle.setPosition(adjustedX, adjustedY);
                circle.setFillColor(pointColor);

                lidarTexture.draw(circle);

                // Add the obstacle to the vector
                lidarObstacles.push_back(Point2f(adjustedX, adjustedY));
            }

            // Compare the obstacles detected by the depth sensor and LiDAR
            compareObstacles(depthObstacles, lidarObstacles);

            lidarTexture.display();
            sf::Sprite lidarSprite(lidarTexture.getTexture());
            lidarSprite.setPosition(10, 10); // Position the minimap at the top-left corner
            window.draw(lidarSprite);

            window.display();
        } else {
            cerr << "Failed to get LiDAR data" << endl;
        }
    }

    // Stop scanning
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
