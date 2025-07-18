#include "SFML/Graphics.hpp"
#include <iostream>
#include <thread>
#include <array>
#include "RandomRange.hpp"
#include "Matrix.hpp"
#include <algorithm>
#include <future>

class Physarum {
	public:
		Physarum(unsigned int);
		Physarum(unsigned int, unsigned int);
		void setCellState(int, int, int);
		void evaluatePhysarum();
		bool getRoute();
	private:
		void physarumThreadFunction(int, int);
		int setCurrentDirection();
		bool isOnCurrentDirection(int, int);
		void findState();
		bool isOnMoore(int, int);
		bool isOnMooreOffset(int);
		std::vector<int> isOnCorner();
		void cleanRouteData();
		void getAllNeighbours(std::vector<int> &, int, int, Matrix&);
		void physarumTransitionConditions(unsigned int, unsigned int, int);
	private:
		RandomRange rnd;
		unsigned int width{ 0 };
		unsigned int height{ 0 };
		int nutrientNotFounded{ 0 };
		int nutrientFounded{ 0 };
		int physarumActualCells{ 0 };
		int physarumLastCells{ 0 };
		int minimumPhysarumCells{ 0 };
		int minimumCheck{ 0 };
		const std::vector<std::pair<int, int>> mooreOffsets = {
			{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}
		};
		Matrix mtxAux;
		const unsigned int threadNumber{ 1 };
		std::vector<std::thread> currentThreads;
		
		unsigned int rowsPerThread{ 0 };

		std::array<bool, 9> statesFounded = {};

		std::vector<int> neighboursData;
		std::vector<int> neighboursDirections;
	public:
		bool allNutrientsFounded{ false };

		bool routed{ false };

		std::array<int, 9> statesDensity = {};

		Matrix mtxPhysarum;
		Matrix mtxMemory;
};


Physarum::Physarum(unsigned int size) : Physarum(size, size) { }

Physarum::Physarum(unsigned int x, unsigned int y) : width(x), height(y) {
	mtxAux.createMatrix(width, height);
	mtxPhysarum.createMatrix(width, height);
	mtxMemory.createMatrix(width, height);
	
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			if (j == 0) {
				mtxPhysarum.setAt(i, j, 2);
			}
			if (i == 0) {
				mtxPhysarum.setAt(i, j, 2);
			}
			if (j == width - 1) {
				mtxPhysarum.setAt(i, j, 2);
			}
			if (i == width - 1) {
				mtxPhysarum.setAt(i, j, 2);
			}
		}
	}
	statesDensity.fill(0);
	neighboursData.reserve(8);
	neighboursDirections.reserve(8);
}


void Physarum::setCellState(int m, int n, int value) {
	mtxPhysarum.setAt(n, m, value);
}

void Physarum::physarumThreadFunction(int startRow, int endRow) {
	for (unsigned int i = startRow; i < endRow; i++) {
		for (unsigned int j = 0; j < width; j++) {
			getAllNeighbours(neighboursData, i, j, mtxPhysarum);
			physarumTransitionConditions(i, j, mtxPhysarum.getAt(i, j));

			neighboursData.clear();
		}
	}
}

void Physarum::evaluatePhysarum() {
	// Cleans states density
	statesDensity.fill(0);

	mtxAux = mtxPhysarum;
	

	for (unsigned int i = 0; i < height; i++) {
		for (unsigned int j = 0; j < width; j++) {

			getAllNeighbours(neighboursData, i, j, mtxPhysarum);
			physarumTransitionConditions(i, j, mtxPhysarum.getAt(i, j));

			neighboursData.clear();
			neighboursDirections.clear();
		}
	}
	
	std::swap(mtxPhysarum, mtxAux);

	// Validate to get route
	if (nutrientNotFounded == 0 && nutrientFounded > 0) {
		allNutrientsFounded = true;

		if (physarumActualCells < physarumLastCells) 
			minimumPhysarumCells = physarumActualCells;
		
		(minimumPhysarumCells == physarumLastCells) ? minimumCheck++ : minimumCheck = 0;
		
		if (minimumCheck > 10) routed = true;
	}
	
	physarumLastCells = physarumActualCells;
	cleanRouteData();
}

void Physarum::physarumTransitionConditions(unsigned int i, unsigned int j, int cell) {
	const auto & cornerCells = isOnCorner();
	// For corners cell (No escape)
	if (!cornerCells.empty()) 
		for (size_t i = 0; i < cornerCells.size(); i++) 
			neighboursData[cornerCells[i]] = 2;
		
	
	const int & currentCellDirection = setCurrentDirection();

	getAllNeighbours(neighboursDirections, i, j, mtxMemory);
	
	findState();

	// Evaluate current state of cell
	switch (cell) {
		case 0:
			if ((isOnCurrentDirection(currentCellDirection, 3) ||
				isOnCurrentDirection(currentCellDirection, 4) ||
				isOnCurrentDirection(currentCellDirection, 6))
				&& mtxMemory.getAt(i, j) == 0) {
				mtxAux.setAt(i, j, 7);
			}
			statesDensity[0]++;
			break;
		case 1:
			if (statesFounded[5] || statesFounded[6]) {
				mtxAux.setAt(i, j, 6);
			}
			nutrientNotFounded++;
			statesDensity[1]++;
			break;
		case 2:
			statesDensity[2]++;
			break;
		case 3:
			statesDensity[3]++;
			break;
		case 4:
			if ((((isOnCurrentDirection(currentCellDirection, 3) ||
				isOnCurrentDirection(currentCellDirection, 5) ||
				isOnCurrentDirection(currentCellDirection, 6)) &&
				mtxMemory.getAt(i, j) == 0 && !statesFounded[0] && !statesFounded[7]))) {

				mtxAux.setAt(i, j, 5);
				mtxMemory.setAt(i, j, currentCellDirection + 1);
			}
			statesDensity[4]++;
			break;
		case 5:
			if (!isOnMooreOffset(5) &&
				!isOnMooreOffset(8) &&
				!statesFounded[1] && !statesFounded[3] && !statesFounded[4] && !statesFounded[6]) {

				mtxAux.setAt(i, j, 0);
				mtxMemory.setAt(i, j, 0);
			}
			else {
				mtxAux.setAt(i, j, 8);
			}
			physarumActualCells++;
			statesDensity[5]++;
			break;
		case 6:
			nutrientFounded++;
			statesDensity[6]++;
			break;
		case 7:
			if (statesFounded[3] || statesFounded[4] || statesFounded[6]) {
				mtxAux.setAt(i, j, 4);
			}
			statesDensity[7]++;
			break;
		case 8:
			mtxAux.setAt(i, j, 5);
			physarumActualCells++;
			statesDensity[8]++;
			break;
		default:
			break;
	}
}

// Clean data that is necesary to finish physarum
void Physarum::cleanRouteData() {
	physarumActualCells = 0;
	nutrientFounded = 0;
	nutrientNotFounded = 0;
}

int Physarum::setCurrentDirection() {
	return rnd.getRandom(0, 7);
}

bool Physarum::isOnCurrentDirection(int direction,int state) {
	if (neighboursData[direction] == state) {
		return true;
	}
	else {
		return false;
	}
}


void Physarum::findState() {
	statesFounded.fill(false);

	for (const auto & n : neighboursData) {
		statesFounded[n] = true;
	}
}

bool Physarum::isOnMoore(int state, int currentDirection) {
	for (size_t i = 0; i < neighboursData.size(); i++) {
		if (i == currentDirection && state == neighboursData[i]) {
			return true;
		}
	}
	return false;
}

bool Physarum::isOnMooreOffset(int state) {
	for (size_t i = 4, j = 0; j < neighboursDirections.size(); i++, j++) {
		if (i > 7) {
			i = 0;
		}
		if (neighboursDirections[i] - 1 == j) {
			return true;
		}
	}
	return false;
}

std::vector<int> Physarum::isOnCorner() {
	std::vector<int> corners;
	if (neighboursData[0] == 2 && neighboursData[2] == 2) {
		corners.push_back(1);
	}
	if (neighboursData[0] == 2 && neighboursData[6] == 2) {
		corners.push_back(7);
	}
	if (neighboursData[6] == 2 && neighboursData[4] == 2) {
		corners.push_back(5);
	}
	if (neighboursData[2] == 2 && neighboursData[4] == 2) {
		corners.push_back(3);
	}
	return corners;
}


bool Physarum::getRoute() {
	int i = 0;
	while (!routed) {
		evaluatePhysarum();
		mtxPhysarum.showConsoleMatrix();
		i++;
	}
	return true;
}



void Physarum::getAllNeighbours(std::vector<int> & data, int i, int j, Matrix & mtx) {	
	for (const auto & [dy, dx] : mooreOffsets) {
		unsigned int x = std::clamp( i + dx, 0, (int) width - 1);
		unsigned int y = std::clamp( j + dy, 0, (int) height - 1);
		data.emplace_back(mtx.getAt(x, y));
	}
}
