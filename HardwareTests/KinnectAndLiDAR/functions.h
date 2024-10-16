#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

// Declaraciones de las funciones que vas a probar
void drawGrid(sf::RenderTarget &target, float gridSpacing, float offsetX, float offsetY);
sf::Color getPointColor(float distance, float maxRange);
void drawNorthIndicator(sf::RenderTexture &lidarTexture, float offsetX, float offsetY);
void detectAndDrawDepthObstacles(cv::Mat &depthMat, sf::RenderTexture &lidarTexture, float offsetX, float offsetY, float scale, std::vector<cv::Point2f> &depthObstacles);
void compareObstacles(const std::vector<cv::Point2f> &depthObstacles, const std::vector<cv::Point2f> &lidarObstacles);

#endif // FUNCTIONS_H
