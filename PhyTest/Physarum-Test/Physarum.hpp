#include "SFML/Graphics.hpp"
#include <iostream>

class Physarum {
	public:
		Physarum(int);
		void setCellState(int, int, int);
		void cleanCells();
		void showPhysarum();
		void evaluatePhysarum();
		void physarumTransitionConditions(int, int, int, std::vector<int>, int**);
	private:
		std::vector<int> neighbours(int, int);
		int setCurrentDirection(bool);
		bool isOnCurrentDirection(std::vector<int>, int, int);
		bool cellConnectCell(std::vector<int>);
		bool findState(std::vector<int>, int);
	private:
		int size = 100;
		bool** cellsMemory;
	public:
		int** cells;
};

Physarum::Physarum(int preSize) {
	size = preSize;
	cells = new int* [size];
	cellsMemory = new bool* [size];
	for (size_t i = 0; i < size; i++) {
		cells[i] = new int[size];
		cellsMemory[i] = new bool[size];
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
			cellsMemory[i][j] = false;
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
	if (value > 0)
		cellsMemory[n][m] = 1;
	else
		cellsMemory[n][m] = 0;
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
			physarumTransitionConditions(i, j, cells[i][j], neighboursData, tab);
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

void Physarum::physarumTransitionConditions(int i, int j, int cell, std::vector<int> neighboursData, int** cellsAux) {
	
	size_t contador = 0;
	int sizeArray = neighboursData.size();
	int currentCellDirection = setCurrentDirection(false);
	
	bool onState0 = findState(neighboursData, 0);
	bool onState1 = findState(neighboursData, 1);
	bool onState3 = findState(neighboursData, 3);
	bool onState4 = findState(neighboursData, 4);
	bool onState5 = findState(neighboursData, 5);
	bool onState6 = findState(neighboursData, 6);
	switch (cell) {
		case 0:
			if ((isOnCurrentDirection(neighboursData, currentCellDirection, 3) ||
				isOnCurrentDirection(neighboursData, currentCellDirection, 4) ||
				isOnCurrentDirection(neighboursData, currentCellDirection, 6)) && cellsMemory == 0) {
				cellsAux[i][j] = 7;
				cellsMemory[i][j] = 1;
			}
			break;
		case 1:
			if (onState5 || onState6) {
				cellsAux[i][j] = 6;
			}
			break;
		case 2:
			cellsAux[i][j] = 2;
			break;
		case 3:
			cellsAux[i][j] = 3;
			break;
		case 4:
			if ((onState3 || onState4 || onState6) && cellsMemory == 0) {
				cellsAux[i][j] = 5;
				cellsMemory[i][j] = 1;
			}
			break;
		case 5:
			
			if (!cellConnectCell(neighboursData))
				cellsAux[i][j] = 8;
			else 
				cellsAux[i][j] = 0;
				
			break;
		case 6:
			cellsAux[i][j] = 6;
			break;
		case 7:
			if (onState5 || onState4 || onState6 || onState3) {
				cellsAux[i][j] = 4;
			}
			break;
		case 8:
			cellsAux[i][j] = 5;
			break;
	default:
		break;
	}
}

int Physarum::setCurrentDirection(bool newmann) {
	int randomValue = 0;
	if (!newmann) 
		randomValue = rand() % 8;
	else 
		randomValue = rand() % 4;
	return randomValue;
}

bool Physarum::isOnCurrentDirection(std::vector<int> neighboursData, int direction,int state) {
	if (neighboursData[direction] == state) {
		return true;
	}
	else {
		return false;
	}
}

bool Physarum::cellConnectCell(std::vector<int> neighboursData) {
	int emptyNeighbour = 0;
	for (size_t i = 0; i < neighboursData.size(); i++) {
		if (neighboursData[i] == 1 || neighboursData[i] == 3 || neighboursData[i] == 4 || neighboursData[i] == 5 || neighboursData[i] == 6 || neighboursData[i] == 8) {
			int nextValue = i + 1;
			int prevValue = i - 1;
			if (i == neighboursData.size() - 1) {
				nextValue = 0;
			}
			else
				if (i == 0) {
					prevValue = neighboursData.size() - 1;
				}
			if ((neighboursData[nextValue] == 0 || neighboursData[nextValue] == 2 || neighboursData[nextValue] == 7) &&
				(neighboursData[prevValue] == 0 || neighboursData[prevValue] == 2) || neighboursData[prevValue] == 7) {
				emptyNeighbour++;
			}
		}
	}
	if (emptyNeighbour > 1) {
		return true;
	}
	else {
		return false;
	}
}

bool Physarum::findState(std::vector<int> neigbours, int state) {
	for (size_t i = 0; i < neigbours.size(); i++) 
		if (neigbours[i] == state)
			return true;
	
	return false;
}

