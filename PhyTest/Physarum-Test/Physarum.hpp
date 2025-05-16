#include "SFML/Graphics.hpp"
#include <iostream>
#include <thread>
#include <array>
#include "RandomRange.hpp"
#include "Matrix.hpp"
#include <algorithm>

class Physarum {
	public:
		Physarum(int);
		void setCellState(int, int, int);
		void evaluatePhysarum();
		void physarumTransitionConditions(int, int, int, std::vector<int>);
		bool getRoute();
	private:
		int setCurrentDirection();
		bool isOnCurrentDirection(int, int, std::vector<int>&);
		bool findState(int, std::vector<int>&);
		bool isOnMoore(int, int, std::vector<int> &);
		bool isOnMooreOffset(std::vector<int> & , int);
		std::vector<int> isOnCorner(std::vector<int> &);
		void cleanRouteData();
		void initializeDensityValues();
		std::vector<int> getAllNeighbours(int, int, Matrix&);
	private:
		int size = 100;
		int nutrientNotFounded = 0;
		int nutrientFounded = 0;
		int physarumActualCells = 0;
		int physarumLastCells = 0;
		int minimumPhysarumCells = 0;
		int minimumCheck = 0;
		const std::vector<std::pair<int, int>> mooreOffsets = {
			{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}
		};
		Matrix mtxAux;
	public:
		bool allNutrientsFounded = false;

		bool routed = false;

		std::array<int, 9> statesDensity = {};

		Matrix mtxPhysarum;
		Matrix mtxMemory;
};

Physarum::Physarum(int preSize) {
	mtxAux.createMatrix(preSize, preSize);
	mtxPhysarum.createMatrix(preSize, preSize);
	mtxMemory.createMatrix(preSize, preSize);

	size = preSize;
	
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			if (j == 0) {
				mtxPhysarum.setAt(i, j, 2);
			}
			if (i == 0) {
				mtxPhysarum.setAt(i, j, 2);
			}
			if (j == size - 1) {
				mtxPhysarum.setAt(i, j, 2);
			}
			if (i == size - 1) {
				mtxPhysarum.setAt(i, j, 2);
			}
		}
	}
	initializeDensityValues();
}

void Physarum::initializeDensityValues() {
	for (size_t i = 0; i < 9; i++) {
		statesDensity[i] = 0;
	}
}


void Physarum::setCellState(int m, int n, int value) {
	mtxPhysarum.setAt(n, m, value);
}


void Physarum::evaluatePhysarum() {
	// Get range and total cells
	const int total_cells = size * size;

	int lastValue = 0;

	// Count each value per state
	initializeDensityValues();

	mtxAux = std::move(mtxPhysarum);

	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			std::vector<int> neighboursData = getAllNeighbours(i, j, mtxPhysarum);
			physarumTransitionConditions(i, j, mtxPhysarum.getAt(i, j), neighboursData);
			
			neighboursData.clear();
		}
	}
	mtxPhysarum = std::move(mtxAux);

	// Validate to get route
	if (nutrientNotFounded == 0 && nutrientFounded > 0) {
		allNutrientsFounded = true;
		if (physarumActualCells < physarumLastCells) {
			minimumPhysarumCells = physarumActualCells;
		}
		if (minimumPhysarumCells == physarumLastCells) {
			minimumCheck++;
		}
		else {
			minimumCheck = 0;
		}
		if (minimumCheck > 10) {
			routed = true;
		}
	}
	
	physarumLastCells = physarumActualCells;
	cleanRouteData();
}

void Physarum::physarumTransitionConditions(int i, int j, int cell, std::vector<int> neighboursData) {
	std::vector<int> cornerCells = isOnCorner(neighboursData);
	// For corners cell (No escape)
	if (!cornerCells.empty()) {
		for (size_t i = 0; i < cornerCells.size(); i++) {
			neighboursData[cornerCells[i]] = 2;
		}
	}
	
	int sizeArray = neighboursData.size();
	int currentCellDirection = setCurrentDirection();

	std::vector<int> neighboursDirections = getAllNeighbours(i, j, mtxMemory);

	const bool onState0 = findState(0, neighboursData);
	const bool onState1 = findState(1, neighboursData);
	const bool onState3 = findState(3, neighboursData);
	const bool onState4 = findState(4, neighboursData);
	const bool onState5 = findState(5, neighboursData);
	const bool onState6 = findState(6, neighboursData);
	const bool onState7 = findState(7, neighboursData);
	
	// Evaluate current state of cell
	switch (cell) {
		case 0:
			if ((isOnCurrentDirection(currentCellDirection, 3, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 4, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 6, neighboursData))
				&& mtxMemory.getAt(i, j) == 0) {
				mtxAux.setAt(i, j, 7);
			}
			statesDensity[0]++;
			break;
		case 1:
			if (onState5 || onState6) {
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
			if ((((isOnCurrentDirection(currentCellDirection, 3, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 5, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 6, neighboursData)) &&
				mtxMemory.getAt(i, j) == 0 && !onState0 && !onState7))) {

				mtxAux.setAt(i, j, 5);
				mtxMemory.setAt(i, j, currentCellDirection + 1);
			}
			statesDensity[4]++;
			break;
		case 5:
			if (!isOnMooreOffset(neighboursDirections, 5) &&
				!isOnMooreOffset(neighboursDirections, 8) &&
				!onState1 && !onState3 && !onState4 && !onState6) {

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
			if (onState3 || onState4 || onState6) {
				mtxAux.setAt(i, j, 4);
			}
			break;
			statesDensity[7]++;
		case 8:
			mtxAux.setAt(i, j, 5);
			physarumActualCells++;
			statesDensity[8]++;
			break;
		default:
			break;
		}
	neighboursDirections.clear();
}

// Clean data that is necesary to finish physarum
void Physarum::cleanRouteData() {
	physarumActualCells = 0;
	nutrientFounded = 0;
	nutrientNotFounded = 0;
}

int Physarum::setCurrentDirection() {
	RandomRange rnd;
	return rnd.getRandom(0, 7);
}

bool Physarum::isOnCurrentDirection(int direction,int state, std::vector<int> & neighboursData) {
	if (neighboursData[direction] == state) {
		return true;
	}
	else {
		return false;
	}
}


bool Physarum::findState(int state, std::vector<int> & neighboursData) {
	for (size_t i = 0; i < neighboursData.size(); i++) 
		if (neighboursData[i] == state)
			return true;
	return false;
}

bool Physarum::isOnMoore(int state, int currentDirection, std::vector<int> & neighboursData) {
	for (size_t i = 0; i < neighboursData.size(); i++) {
		if (i == currentDirection && state == neighboursData[i]) {
			return true;
		}
	}
	return false;
}

bool Physarum::isOnMooreOffset(std::vector<int> & neighboursDirections, int state) {
	bool finished = false;
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

std::vector<int> Physarum::isOnCorner(std::vector<int> & neighboursData) {
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
		i++;
	}
	return true;
}



std::vector<int> Physarum::getAllNeighbours(int i, int j, Matrix & mtx) {
	std::vector<int> data;
		
	for (const auto & [dy, dx] : mooreOffsets) {
		int x = (i + dx + size) % size;
		int y = (j + dy + size) % size;
		data.push_back(mtx.getAt(x, y));
	}

	return data;
}
