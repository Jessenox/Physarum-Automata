#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

class LoadMap {
	public:
		void convertImageToMap(std::string);
		void setDataToArray(int**, int, int);
	private:
		void grayscaleImage();
		void setDataToRGBVector(int, int);
	public:
		float isImgProcessed = false;
	private:
		cv::Mat actualImage;
		cv::Mat processedImage;
		std::vector<std::vector<float>> rgbVector;
		float threshold = 150;
		int IMG_WIDTH = 50, IMG_HEIGHT = 50;
};

void LoadMap::setDataToRGBVector(int rows, int cols) {
	for (size_t i = 0; i < cols; i++) {
		std::vector<float> aux;
		for (size_t j = 0; j < rows; j++) {
			aux.push_back(processedImage.at<uchar>(i, j));
		}
		rgbVector.push_back(aux);
	}
}

void LoadMap::grayscaleImage() {
	cv::Mat tmpImg;
	cv::resize(actualImage, tmpImg, cv::Size(IMG_WIDTH, IMG_HEIGHT), cv::INTER_LINEAR);
	cv::cvtColor(tmpImg, processedImage, cv::COLOR_RGB2GRAY);
}

void LoadMap::setDataToArray(int **array, int height, int width) {
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			if (rgbVector[i][j] > threshold) {
				array[i][j] = 2;
			}
			else {
				array[i][j] = 0;
			}
			if (i == 0 || j == 0 || i == height - 1 || j == width - 1) {
				array[i][j] = 2;
			}
		}
	}
	imshow("Display window", processedImage);
}

void LoadMap::convertImageToMap(std::string image_url) {
	actualImage = cv::imread(image_url, cv::IMREAD_COLOR);
	grayscaleImage();
	setDataToRGBVector(processedImage.rows, processedImage.cols);
	
	isImgProcessed = true;
}