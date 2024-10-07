#include "SFML/Graphics.hpp"
#include <iostream>
#include <thread>
#include "RandomRange.hpp"

#if defined (_MSC_VER)  // Visual studio
#define thread_local __declspec( thread )
#elif defined (__GCC__) // GCC
#define thread_local __thread
#endif

class Physarum {
	public:
		Physarum(int);
		void setCellState(int, int, int);
		void cleanCells();
		void showPhysarum();
		void evaluatePhysarum();
		void physarumTransitionConditions(int, int, int, std::vector<int>);
		bool getRoute();
	private:
		std::vector<int> getNeighbours(int, int, int**);
		int setCurrentDirection();
		bool isOnCurrentDirection(int, int, std::vector<int>);
		bool findState(int, std::vector<int>);
		bool isOnMoore(int, int, std::vector<int>);
		bool isOnMooreOffset(std::vector<int>, int);
		std::vector<int> isOnCorner(std::vector<int>);
		void cleanRouteData();
		void threadableCalculation(int, int);
		void resetAuxArray();
		void matchArrays();
		void initializeDensityValues();
	private:
		int size = 100;
		int** cellsAux;
		int nutrientNotFounded = 0;
		int nutrientFounded = 0;
		int physarumActualCells = 0;
		int physarumLastCells = 0;
		int minimumPhysarumCells = 0;
		int minimumCheck = 0;
	public:
		bool allNutrientsFounded = false;

		bool routed = false;
		int** cells;
		int** cellsMemory;
		int statesDensity[9];
};

Physarum::Physarum(int preSize) {
	size = preSize;
	cells = new int* [size];
	cellsMemory = new int* [size];
	cellsAux = new int* [size];

	for (size_t i = 0; i < size; i++) {
		cells[i] = new int[size];
		cellsMemory[i] = new int[size];
		cellsAux[i] = new int[size];
		for (size_t j = 0; j < size; j++) {
			cells[i][j] = 0;
			cellsAux[i][j] = 0;
		}
	}
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			if (j == 0) {
				cells[i][j] = 2;
			}
			if (i == 0) {
				cells[i][j] = 2;
			}
			if (j == size - 1) {
				cells[i][j] = 2;
			}
			if (i == size - 1) {
				cells[i][j] = 2;
			}
			cellsMemory[i][j] = 0;
		}
	}
	initializeDensityValues();
}

void Physarum::initializeDensityValues() {
	for (size_t i = 0; i < 9; i++) {
		statesDensity[i] = 0;
	}
}

void Physarum::cleanCells() {
	if (cells != NULL) {
		for (int i = 0; i < size; i++) {
			delete[] cells[i];
			delete[] cellsAux[i];
		}
		delete[] cells;
		delete[] cellsAux;
	}
}

void Physarum::setCellState(int m, int n, int value) {
	cells[n][m] = value;
	if (value == 0)
		cellsMemory [n][m] = 0;
}

void Physarum::resetAuxArray() {
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			cellsAux[i][j] = 0;
		}
	}
}

void Physarum::matchArrays() {
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			cellsAux[i][j] = cells[i][j];
		}
	}
}

void Physarum::evaluatePhysarum() {
	matchArrays();
	// Using threads to calculate Physarum
	// First, how many threads are available to use
	const int n_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> work_threads;
	// Get range and total cells
	const int total_cells = size * size;
	const int evaluation_range = total_cells / n_threads;
	int lastValue = 0;

	// Count each value per state
	initializeDensityValues();
	
	/*

	std::thread my_thread_1(&Physarum::threadableCalculation, this, 0, 20);
	std::thread my_thread_2(&Physarum::threadableCalculation, this, 20, 40);
	std::thread my_thread_3(&Physarum::threadableCalculation, this, 40, 60);
	std::thread my_thread_4(&Physarum::threadableCalculation, this, 60, 80);
	std::thread my_thread_5(&Physarum::threadableCalculation, this, 80, 100);
	*/
	// Evaluate each cell for next generation

	
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			std::vector<int> neighboursData = getNeighbours(j, i, cells);
			physarumTransitionConditions(i, j, cells[i][j], neighboursData);
			cells[i][j] = cellsAux[i][j];
			neighboursData.clear();
		}
	}
	
	/*
	my_thread_1.join();
	my_thread_2.join();
	my_thread_3.join();
	my_thread_4.join();
	my_thread_5.join();
	*/
	//threadableCalculation(tab, 50, 50);



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
	
	resetAuxArray();

	physarumLastCells = physarumActualCells;
	cleanRouteData();
}

void Physarum::showPhysarum() {
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			std::cout << " " << cells[i][j];
		}
		std::cout << "\n";
	}
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
	std::vector<int> neighboursDirections = getNeighbours(j, i, cellsMemory);
	
	bool onState0 = findState(0, neighboursData);
	bool onState1 = findState(1, neighboursData);
	bool onState3 = findState(3, neighboursData);
	bool onState4 = findState(4, neighboursData);
	bool onState5 = findState(5, neighboursData);
	bool onState6 = findState(6, neighboursData);
	bool onState7 = findState(7, neighboursData);
	
	// Evaluate current state of cell
	switch (cell) {
		case 0:
			if ((isOnCurrentDirection(currentCellDirection, 3, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 4, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 6, neighboursData))
				&& cellsMemory[i][j] == 0) {
				cellsAux[i][j] = 7;
			}
			statesDensity[0]++;
			break;
		case 1:
			if (onState5 || onState6) {
				cellsAux[i][j] = 6;
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
				cellsMemory[i][j] == 0 && !onState0 && !onState7))) {
				cellsAux[i][j] = 5;
				cellsMemory[i][j] = currentCellDirection + 1;
			}
			statesDensity[4]++;
			break;
		case 5:
			if (!isOnMooreOffset(neighboursDirections, 5) &&
				!isOnMooreOffset(neighboursDirections, 8) &&
				!onState1 && !onState3 && !onState4 && !onState6) {
				cellsAux[i][j] = 0;
				cellsMemory[i][j] = 0;
			}
			else {
				cellsAux[i][j] = 8;
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
				cellsAux[i][j] = 4;
			}
			break;
			statesDensity[7]++;
		case 8:
			cellsAux[i][j] = 5;
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

bool Physarum::isOnCurrentDirection(int direction,int state, std::vector<int> neighboursData) {
	if (neighboursData[direction] == state) {
		return true;
	}
	else {
		return false;
	}
}


bool Physarum::findState(int state, std::vector<int> neighboursData) {
	for (size_t i = 0; i < neighboursData.size(); i++) 
		if (neighboursData[i] == state)
			return true;
	
	return false;
}

bool Physarum::isOnMoore(int state, int currentDirection, std::vector<int> neighboursData) {
	for (size_t i = 0; i < neighboursData.size(); i++) {
		if (i == currentDirection && state == neighboursData[i]) {
			return true;
		}
	}
	return false;
}

bool Physarum::isOnMooreOffset(std::vector<int> neighboursDirections, int state) {
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

std::vector<int> Physarum::isOnCorner(std::vector<int> neighboursData) {
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

void Physarum::threadableCalculation(int start, int end) {
	for (size_t i = start; i < end; i++) {
		for (size_t j = 0; j < size; j++) {
			std::vector<int> neighboursData = getNeighbours(j, i, cells);
			physarumTransitionConditions(i, j, cells[i][j], neighboursData);
			cells[i][j] = cellsAux[i][j];
			neighboursData.clear();
		}
	}
	
}

bool Physarum::getRoute() {
	int i = 0;
	while (!routed) {
		evaluatePhysarum();
		i++;
	}
	return true;
}

std::vector<int> Physarum::getNeighbours(int j, int i, int ** actualArray) {
	std::vector<int> data;
	if (i == 0) {
		data.push_back(actualArray[size - 1][j]);
		if (j == size - 1) {
			data.push_back(actualArray[size - 1][0]);
			data.push_back(actualArray[i][0]);
			data.push_back(actualArray[i + 1][0]);
		}
		else {
			data.push_back(actualArray[size - 1][j + 1]);
			data.push_back(actualArray[i][j + 1]);
			data.push_back(actualArray[i + 1][j + 1]);
		}
		data.push_back(actualArray[i + 1][j]);
		if (j == 0) {
			data.push_back(actualArray[i + 1][size - 1]);
			data.push_back(actualArray[i][size - 1]);
			data.push_back(actualArray[size - 1][size - 1]);
		}
		else {
			data.push_back(actualArray[i + 1][j - 1]);
			data.push_back(actualArray[i][j - 1]);
			data.push_back(actualArray[size - 1][j - 1]);
		}

	}
	else if (i == size - 1) {
		data.push_back(actualArray[i - 1][j]);
		if (j == size - 1) {
			data.push_back(actualArray[i - 1][0]);
			data.push_back(actualArray[i][0]);
			data.push_back(actualArray[0][0]);

		}
		else {
			data.push_back(actualArray[i - 1][j + 1]);
			data.push_back(actualArray[i][j + 1]);
			data.push_back(actualArray[0][j + 1]);
		}
		data.push_back(actualArray[0][j]);
		if (j == 0) {
			data.push_back(actualArray[0][size - 1]);
			data.push_back(actualArray[i][size - 1]);
			data.push_back(actualArray[i - 1][size - 1]);
		}
		else {
			data.push_back(actualArray[0][j - 1]);
			data.push_back(actualArray[i][j - 1]);
			data.push_back(actualArray[i - 1][j - 1]);
		}
	}
	else {
		data.push_back(actualArray[i - 1][j]);
		if (j == size - 1) {
			data.push_back(actualArray[i - 1][0]);
			data.push_back(actualArray[i][0]);
			data.push_back(actualArray[i + 1][0]);
		}
		else {
			data.push_back(actualArray[i - 1][j + 1]);
			data.push_back(actualArray[i][j + 1]);
			data.push_back(actualArray[i + 1][j + 1]);
		}
		data.push_back(actualArray[i + 1][j]);
		if (j == 0) {
			data.push_back(actualArray[i + 1][size - 1]);
			data.push_back(actualArray[i][size - 1]);
			data.push_back(actualArray[i - 1][size - 1]);
		}
		else {
			data.push_back(actualArray[i + 1][j - 1]);
			data.push_back(actualArray[i][j - 1]);
			data.push_back(actualArray[i - 1][j - 1]);
		}
	}
	return data;
}

