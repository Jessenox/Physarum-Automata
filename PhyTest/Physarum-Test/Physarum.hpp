#include "SFML/Graphics.hpp"
#include <iostream>

class Physarum {
	public:
		Physarum(int);
		void setCellState(int, int, int);
		void cleanCells();
		void showPhysarum();
		void evaluatePhysarum();
		void physarumTransitionConditions(int, int, int, int**, std::vector<int>);
		bool getRoute();
	private:
		std::vector<int> neighbours(int, int);
		std::vector<int> neighboursMemory(int, int);
		std::vector<int> getNewmannNeighbours(int, int);
		int setCurrentDirection(bool);
		bool isOnCurrentDirection(int, int, std::vector<int>);
		bool findState(int, std::vector<int>);
		bool isOnMoore(int, int, std::vector<int>);
		bool isOnMooreOffset(std::vector<int>, int);
		int isOnCorner(std::vector<int>);
		void cleanRouteData();
	private:
		int size = 100;
		int** cellsMemory;
		int nutrientNotFounded = 0;
		int nutrientFounded = 0;
		int physarumActualCells = 0;
		int physarumLastCells = 0;
		int minimumPhysarumCells = 0;
		int minimumCheck = 0;
	public:
		bool routed = false;
		int** cells;
};

Physarum::Physarum(int preSize) {
	size = preSize;
	cells = new int* [size];
	cellsMemory = new int* [size];
	for (size_t i = 0; i < size; i++) {
		cells[i] = new int[size];
		cellsMemory[i] = new int[size];
		for (size_t j = 0; j < size; j++) {
			cells[i][j] = 0;
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
}

void Physarum::cleanCells() {
	if (cells != NULL) {
		for (int i = 0; i < size; i++)
			delete[] cells[i];
		delete[] cells;
	}
}

void Physarum::setCellState(int m, int n, int value) {
	cells[n][m] = value;
}

void Physarum::evaluatePhysarum() {

	int** tab = new int* [size];
	for (size_t i = 0; i < size; i++) {
		tab[i] = new int[size];
		for (size_t j = 0; j < size; j++) {
			tab[i][j] = cells[i][j];
		}
	}

	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			std::vector<int> neighboursData = neighbours(j, i);
			physarumTransitionConditions(i, j, cells[i][j], tab, neighboursData);
			neighboursData.clear();
		}
	}
	// Validate to get route
	if (nutrientNotFounded == 0 && nutrientFounded > 0) {
		if (physarumActualCells < physarumLastCells) {
			minimumPhysarumCells = physarumActualCells;
		}
		if (minimumPhysarumCells == physarumLastCells) {
			minimumCheck++;
		}
		else {
			minimumCheck = 0;
		}
		if (minimumCheck > 5) {
			routed = true;
		}
	}

	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			cells[i][j] = tab[i][j];
		}
	}
	
	if (tab != NULL) {
		for (int i = 0; i < size; i++)
			delete[] tab[i];
		delete[] tab;
	}
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
	std::cout << "-----Physarum Memmory:-------\n";
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			std::cout << " " << cellsMemory[i][j];
		}
		std::cout << "\n";
	}
}

void Physarum::physarumTransitionConditions(int i, int j, int cell, int** cellsAux, std::vector<int> neighboursData) {
	int cornerCell = isOnCorner(neighboursData);
	
	if (cornerCell != -1) {
		neighboursData[cornerCell] = 2;
	}
	

	size_t contador = 0;
	
	for (size_t i = 0; i < neighboursData.size() ; i++) {
		if (neighboursData[i] == 5 || neighboursData[i] == 8)
			contador++;
	}
	
	int sizeArray = neighboursData.size();
	int currentCellDirection = setCurrentDirection(false);
	std::vector<int> neighboursDirections = neighboursMemory(j, i);
	
	bool onState0 = findState(0, neighboursData);
	bool onState1 = findState(1, neighboursData);
	bool onState3 = findState(3, neighboursData);
	bool onState4 = findState(4, neighboursData);
	bool onState5 = findState(5, neighboursData);
	bool onState6 = findState(6, neighboursData);
	bool onState7 = findState(7, neighboursData);
	
	
	switch (cell) {
		case 0:
			if ((isOnCurrentDirection(currentCellDirection, 3, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 4, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 6, neighboursData))
				&& cellsMemory[i][j] == 0) {
				cellsAux[i][j] = 7;
			}
			break;
		case 1:
			if (onState5 || onState6) {
				cellsAux[i][j] = 6;
			}
			nutrientNotFounded++;
			break;
		case 4:
			if ((isOnCurrentDirection(currentCellDirection, 3, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 5, neighboursData) ||
				isOnCurrentDirection(currentCellDirection, 6, neighboursData)) &&
				cellsMemory[i][j] == 0 && !onState0 && !onState7) {
				cellsAux[i][j] = 5;
				cellsMemory[i][j] = currentCellDirection + 1;
			}
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
			break;
		case 6:
			nutrientFounded++;
			break;
		case 7:
			if (onState3 || onState4 || onState6) {
				cellsAux[i][j] = 4;
			}
			break;
		case 8:
			cellsAux[i][j] = 5;
			physarumActualCells++;
			break;
		default:
			break;
		}
	
	
	neighboursDirections.clear();
}

void Physarum::cleanRouteData() {
	physarumActualCells = 0;
	nutrientFounded = 0;
	nutrientNotFounded = 0;
}

int Physarum::setCurrentDirection(bool newmann) {
	int randomValue = 0;
	if (!newmann) 
		randomValue = rand() % 8;
	else 
		randomValue = rand() % 4;
	return randomValue;
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

int Physarum::isOnCorner(std::vector<int> neighboursData) {
	if (neighboursData[0] == 2 && neighboursData[2] == 2) {
		return 1;
	}
	if (neighboursData[0] == 2 && neighboursData[6] == 2) {
		return 7;
	}
	if (neighboursData[6] == 2 && neighboursData[4] == 2) {
		return 5;
	}
	if (neighboursData[2] == 2 && neighboursData[4] == 2) {
		return 3;
	}
	return -1;
}

std::vector<int> Physarum::neighbours(int x, int y) {
	std::vector<int> data;
	int i = 0, j = 0;
	if (y == 0) {
		i = y, j = x;
		data.push_back(cells[size - 1][j]);
		if (j == size - 1) {
			data.push_back(cells[size - 1][0]);
			data.push_back(cells[i][0]);
			data.push_back(cells[i + 1][0]);
		}
		else {
			data.push_back(cells[size - 1][j + 1]);
			data.push_back(cells[i][j + 1]);
			data.push_back(cells[i + 1][j + 1]);
		}
		data.push_back(cells[i + 1][j]);
		if (j == 0) {
			data.push_back(cells[i + 1][size - 1]);
			data.push_back(cells[i][size - 1]);
			data.push_back(cells[size - 1][size - 1]);
		}
		else {
			data.push_back(cells[i + 1][j - 1]);
			data.push_back(cells[i][j - 1]);
			data.push_back(cells[size - 1][j - 1]);
		}

	}
	else if (y == size - 1) {
		i = y, j = x;
		data.push_back(cells[i - 1][j]);
		if (j == size - 1) {
			data.push_back(cells[i - 1][0]);
			data.push_back(cells[i][0]);
			data.push_back(cells[0][0]);

		}
		else {
			data.push_back(cells[i - 1][j + 1]);
			data.push_back(cells[i][j + 1]);
			data.push_back(cells[0][j + 1]);
		}
		data.push_back(cells[0][j]);
		if (j == 0) {
			data.push_back(cells[0][size - 1]);
			data.push_back(cells[i][size - 1]);
			data.push_back(cells[i - 1][size - 1]);
		}
		else {
			data.push_back(cells[0][j - 1]);
			data.push_back(cells[i][j - 1]);
			data.push_back(cells[i - 1][j - 1]);
		}


	}
	else {
		i = y, j = x;
		data.push_back(cells[i - 1][j]);
		if (j == size - 1) {
			data.push_back(cells[i - 1][0]);
			data.push_back(cells[i][0]);
			data.push_back(cells[i + 1][0]);
		}
		else {
			data.push_back(cells[i - 1][j + 1]);
			data.push_back(cells[i][j + 1]);
			data.push_back(cells[i + 1][j + 1]);
		}
		data.push_back(cells[i + 1][j]);
		if (j == 0) {
			data.push_back(cells[i + 1][size - 1]);
			data.push_back(cells[i][size - 1]);
			data.push_back(cells[i - 1][size - 1]);
		}
		else {
			data.push_back(cells[i + 1][j - 1]);
			data.push_back(cells[i][j - 1]);
			data.push_back(cells[i - 1][j - 1]);
		}
	}
	return data;
}

std::vector<int> Physarum::neighboursMemory(int x, int y) {
	std::vector<int> data;
	int i = 0, j = 0;
	if (y == 0) {
		i = y, j = x;
		data.push_back(cellsMemory[size - 1][j]);
		if (j == size - 1) {
			data.push_back(cellsMemory[size - 1][0]);
			data.push_back(cellsMemory[i][0]);
			data.push_back(cellsMemory[i + 1][0]);
		}
		else {
			data.push_back(cellsMemory[size - 1][j + 1]);
			data.push_back(cellsMemory[i][j + 1]);
			data.push_back(cellsMemory[i + 1][j + 1]);
		}
		data.push_back(cellsMemory[i + 1][j]);
		if (j == 0) {
			data.push_back(cellsMemory[i + 1][size - 1]);
			data.push_back(cellsMemory[i][size - 1]);
			data.push_back(cellsMemory[size - 1][size - 1]);
		}
		else {
			data.push_back(cellsMemory[i + 1][j - 1]);
			data.push_back(cellsMemory[i][j - 1]);
			data.push_back(cellsMemory[size - 1][j - 1]);
		}

	}
	else if (y == size - 1) {
		i = y, j = x;
		data.push_back(cellsMemory[i - 1][j]);
		if (j == size - 1) {
			data.push_back(cellsMemory[i - 1][0]);
			data.push_back(cellsMemory[i][0]);
			data.push_back(cellsMemory[0][0]);

		}
		else {
			data.push_back(cellsMemory[i - 1][j + 1]);
			data.push_back(cellsMemory[i][j + 1]);
			data.push_back(cellsMemory[0][j + 1]);
		}
		data.push_back(cellsMemory[0][j]);
		if (j == 0) {
			data.push_back(cellsMemory[0][size - 1]);
			data.push_back(cellsMemory[i][size - 1]);
			data.push_back(cellsMemory[i - 1][size - 1]);
		}
		else {
			data.push_back(cellsMemory[0][j - 1]);
			data.push_back(cellsMemory[i][j - 1]);
			data.push_back(cellsMemory[i - 1][j - 1]);
		}


	}
	else {
		i = y, j = x;
		data.push_back(cellsMemory[i - 1][j]);
		if (j == size - 1) {
			data.push_back(cellsMemory[i - 1][0]);
			data.push_back(cellsMemory[i][0]);
			data.push_back(cellsMemory[i + 1][0]);
		}
		else {
			data.push_back(cellsMemory[i - 1][j + 1]);
			data.push_back(cellsMemory[i][j + 1]);
			data.push_back(cellsMemory[i + 1][j + 1]);
		}
		data.push_back(cellsMemory[i + 1][j]);
		if (j == 0) {
			data.push_back(cellsMemory[i + 1][size - 1]);
			data.push_back(cellsMemory[i][size - 1]);
			data.push_back(cellsMemory[i - 1][size - 1]);
		}
		else {
			data.push_back(cellsMemory[i + 1][j - 1]);
			data.push_back(cellsMemory[i][j - 1]);
			data.push_back(cellsMemory[i - 1][j - 1]);
		}
	}
	return data;
}

std::vector<int> Physarum::getNewmannNeighbours(int i, int j) {
	std::vector <int> data;
	if (i != 0 || j != 0 || i != size - 1 || j != size - 1) {
		data.push_back(cells[i - 1][j]);
		data.push_back(cells[i][j + 1]);
		data.push_back(cells[i + 1][j]);
		data.push_back(cells[i][j - 1]);
	}
	return data;
}

bool Physarum::getRoute() {
	while (!routed) {
		evaluatePhysarum();
	}
	return true;
}

