#include "utils.h"
#include <cmath>

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
    return sf::Color(static_cast<sf::Uint8>(std::round(255 * (1 - ratio))),
                     static_cast<sf::Uint8>(std::round(255 * ratio)), 
                     0);
}


void drawNorthIndicator(sf::RenderTexture &lidarTexture, float offsetX, float offsetY) {
    sf::Color northColor(255, 0, 0); // Rojo para el indicador norte
    float length = 50.0f; // Longitud de la línea

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(offsetX, offsetY), northColor),
        sf::Vertex(sf::Vector2f(offsetX, offsetY - length), northColor) // Línea hacia arriba
    };
    lidarTexture.draw(line, 2, sf::Lines);
}

void detectAndDrawDepthObstacles(cv::Mat &depthMat, sf::RenderTexture &lidarTexture, float offsetX, float offsetY, float scale, std::vector<cv::Point2f> &depthObstacles) {
    for (int y = 0; y < depthMat.rows; y += 10) {
        for (int x = 0; x < depthMat.cols; x += 10) {
            uint16_t depth_value = depthMat.at<uint16_t>(y, x);
            if (depth_value > 0 && depth_value < 2047) {
                float depth_in_meters = depth_value / 1000.0f;

                float adjustedX = offsetX + (x - depthMat.cols / 2) * scale / depth_in_meters;
                float adjustedY = offsetY - (y - depthMat.rows / 2) * scale / depth_in_meters;

                sf::CircleShape circle(3); // Pequeño círculo para representar el obstáculo
                circle.setPosition(adjustedX, adjustedY);
                circle.setFillColor(sf::Color::Red);

                lidarTexture.draw(circle);

                depthObstacles.push_back(cv::Point2f(adjustedX, adjustedY));
            }
        }
    }
}

void compareObstacles(const std::vector<cv::Point2f> &depthObstacles, const std::vector<cv::Point2f> &lidarObstacles) {
    for (const auto &d : depthObstacles) {
        bool matchFound = false;
        for (const auto &l : lidarObstacles) {
            float distance = sqrt(pow(d.x - l.x, 2) + pow(d.y - l.y, 2));
            if (distance < 10.0f) { // Considerar coincidencia si la distancia es menor que el umbral
                matchFound = true;
                break;
            }
        }
    }
}
