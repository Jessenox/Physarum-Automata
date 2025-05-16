
#include <vector>

class Matrix {
public:
	Matrix(unsigned int width, unsigned int height) : uiWidth(width), uiHeight(height) {
		usvMatrix.assign( uiWidth * uiHeight, 0);
	};

	unsigned short getAt(unsigned int i, unsigned int j);
	void setAt(unsigned int i, unsigned int j, unsigned short value);

	~Matrix();

	unsigned int uiWidth{ 0 };
	unsigned int uiHeight{ 0 };
	std::vector<unsigned short> usvMatrix;
private:
	int a{ 0 };
};

unsigned short Matrix::getAt(unsigned int i, unsigned int j) {
	return usvMatrix.at( (i * uiWidth) + j );
};

void Matrix::setAt(unsigned int i, unsigned int j, unsigned short value) {
	usvMatrix.at((i * uiWidth) + j) = value;
};

Matrix::~Matrix() {
	usvMatrix.clear();
}