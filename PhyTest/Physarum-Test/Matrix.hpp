#pragma  once
#include <vector>

class Matrix {
public:
	Matrix(unsigned int width, unsigned int height) : uiWidth(width), uiHeight(height) {
		usvMatrix.assign(uiWidth * uiHeight, 0);
		std::cout << "w: " << uiWidth << ", h:" << uiHeight << std::endl;
	};
	Matrix() {};
	~Matrix();

	unsigned short getAt(unsigned int i, unsigned int j);
	void setAt(unsigned int i, unsigned int j, unsigned short value);
	void createMatrix(unsigned int width, unsigned int height);
	void showConsoleMatrix();

	unsigned int uiWidth{ 0 };
	unsigned int uiHeight{ 0 };
	std::vector<unsigned short> usvMatrix;
private:
	int a{ 0 };

};

unsigned short Matrix::getAt(unsigned int i, unsigned int j) {
	return usvMatrix.at((i * uiWidth) + j);
};

void Matrix::setAt(unsigned int i, unsigned int j, unsigned short value) {
	usvMatrix.at((i * uiWidth) + j) = value;
};

Matrix::~Matrix() {
	usvMatrix.clear();
}

void Matrix::createMatrix(unsigned int width, unsigned int height) {
	uiWidth = width;
	uiHeight = height;
	usvMatrix.assign(uiWidth * uiHeight, 0);
}

void Matrix::showConsoleMatrix() {
	for (size_t i = 0; i < uiWidth; i++) {
		for (size_t j = 0; j < uiHeight; j++) {
			printf("%d ", usvMatrix.at((i * uiWidth) + j));
		}
		printf("\n");
	}
}