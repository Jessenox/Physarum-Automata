#include "SFML/Graphics.hpp"
#include <iostream>

class Physarum {
	public:
		Physarum(int);
		void setCellState(int, int, int);
		void cleanCells();
		void showPhysarum();
		void evaluatePhysarum();
		void physarumTransitionConditions(int, int, int, int**);
	private:
		std::vector<int> neighbours(int, int);
		std::vector<int> neighboursMemory(int, int);
		int setCurrentDirection(bool);
		bool isOnCurrentDirection(int, int);
		bool cellConnectCell();
		bool findState(int);
		bool isOnMoore(int, int);
		bool isOnMooreOffset(std::vector<int>, int);
	private:
		std::vector<int> neighboursData;
		int size = 100;
		int** cellsMemory;
	public:
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
			neighboursData = neighbours(j, i);
			physarumTransitionConditions(i, j, cells[i][j], tab);
			neighboursData.clear();
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

void Physarum::physarumTransitionConditions(int i, int j, int cell, int** cellsAux) {
	
	size_t contador = 0;
	int sizeArray = neighboursData.size();
	int currentCellDirection = setCurrentDirection(false);
	std::vector<int> neighboursDirections = neighboursMemory(j, i);
	
	bool onState0 = findState(0);
	bool onState1 = findState(1);
	bool onState3 = findState(3);
	bool onState4 = findState(4);
	bool onState5 = findState(5);
	bool onState6 = findState(6);
	bool onState7 = findState(7);

	switch (cell) {
		case 0:
			if ((isOnCurrentDirection(currentCellDirection, 3) ||
				 isOnCurrentDirection(currentCellDirection, 4) || 
				 isOnCurrentDirection(currentCellDirection, 6))
				 && cellsMemory[i][j] == 0) {
				cellsAux[i][j] = 7;
			}
			break;
		case 1:
			if (onState5 || onState6) {
				cellsAux[i][j] = 6;
			}
			break;
		case 4:
			if ((isOnCurrentDirection(currentCellDirection, 3) ||
				 isOnCurrentDirection(currentCellDirection, 5) ||
				 isOnCurrentDirection(currentCellDirection, 6)) &&
				 cellsMemory[i][j] == 0 && !onState0 && !onState7) {
				cellsAux[i][j] = 5;
				cellsMemory[i][j] = currentCellDirection + 1;
			} 
			break;
		case 5:
			if (!isOnMooreOffset(neighboursDirections, 5) && !isOnMooreOffset(neighboursDirections, 8) && !onState1 && !onState3 && !onState4 && !onState6) {
				cellsAux[i][j] = 0;
				cellsMemory[i][j] = 0;
			}
			else {
				cellsAux[i][j] = 8;
			}
			
			break;
		case 7:
			if (onState3 || onState4 || onState5 || onState6) {
				cellsAux[i][j] = 4;
			}
			break;
		case 8:
			cellsAux[i][j] = 5;
			break;
	default:
		break;
	}
	neighboursDirections.clear();
}

int Physarum::setCurrentDirection(bool newmann) {
	int randomValue = 0;
	if (!newmann) 
		randomValue = rand() % 8;
	else 
		randomValue = rand() % 4;
	return randomValue;
}

bool Physarum::isOnCurrentDirection(int direction,int state) {
	if (neighboursData[direction] == state) {
		return true;
	}
	else {
		return false;
	}
}

bool Physarum::cellConnectCell() {
	return true;
}

bool Physarum::findState(int state) {
	for (size_t i = 0; i < neighboursData.size(); i++) 
		if (neighboursData[i] == state)
			return true;
	
	return false;
}

bool Physarum::isOnMoore(int state, int currentDirection) {
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



