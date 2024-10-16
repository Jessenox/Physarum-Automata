#include <gtest/gtest.h>
#include "utils.h"
#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>

// Prueba para la función drawGrid
TEST(DrawGridTest, DrawGridSuccess) {
    sf::RenderTexture renderTexture;
    renderTexture.create(100, 100);

    ASSERT_NO_THROW(drawGrid(renderTexture, 10.0f, 50.0f, 50.0f));
}

// Prueba para la función getPointColor
TEST(GetPointColorTest, PointColorSuccess) {
    sf::Color color = getPointColor(32.0f, 64.0f);
    EXPECT_EQ(color.r, 128);
    EXPECT_EQ(color.g, 128);
    EXPECT_EQ(color.b, 0);
}

// Prueba para la función drawNorthIndicator
TEST(DrawNorthIndicatorTest, DrawNorthIndicatorSuccess) {
    sf::RenderTexture renderTexture;
    renderTexture.create(100, 100);

    ASSERT_NO_THROW(drawNorthIndicator(renderTexture, 50.0f, 50.0f));
}
