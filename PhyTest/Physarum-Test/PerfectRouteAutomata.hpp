#include <vector>
#include <iostream>

class PerfectRouteAutomata {
public:
	void getPrevRoute(int**, int);
private:
	void evaluateAutomata(int**, int);
	void prevRules(int, int, int**, int n, int**);
	std::vector<int> getNeighbours(int j, int i, int** actualArray, int size);
	std::vector<std::tuple<int, int>> getOrderedCoords(int **, int);
	void getOrderedCoord(int**, int**, int, int, int, std::vector<std::tuple<int, int>>*);
public:
private:
	bool endAutomata = false;
	int cellsCounter = 0;
	int** result;
	int coordsCounter = 0;
	bool coordsObtained = false;
};

void PerfectRouteAutomata::getPrevRoute(int** tab, int n) {
	result = new int* [n];
	for (size_t i = 0; i < n; i++) {
		result[i] = new int[n];
		for (size_t j = 0; j < n; j++) {
			result[i][j] = tab[i][j];
		}
	}
	do {
		evaluateAutomata(result, n);
	} while (!endAutomata);

	std::vector<std::tuple<int, int>> myCoords = getOrderedCoords(result, n);

	/*
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n; j++) {
			std::cout << result[i][j] << " ";
		}
		std::cout << "\n";
	}
	*/
}

void PerfectRouteAutomata::evaluateAutomata(int** tab, int n) {
	int** cellsAux = new int* [n];
	for (size_t i = 0; i < n; i++) {
		cellsAux[i] = new int[n];
		for (size_t j = 0; j < n; j++) {
			cellsAux[i][j] = 0;
		}
	}

	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n; j++) {
			prevRules(i, j, tab, n, cellsAux);
		}
	}
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n; j++) {
			tab[i][j] = cellsAux[i][j];
		}
	}


	if (cellsAux != NULL) {
		for (int i = 0; i < n; i++) {
			delete[] cellsAux[i];
		}
		delete[] cellsAux;
	}

	if (cellsCounter == 0)
		endAutomata = true;
	cellsCounter = 0;
}

std::vector<std::tuple<int, int>> PerfectRouteAutomata::getOrderedCoords(int** prevRoute, int n) {
	std::vector<std::tuple<int, int>> coords;
	bool initialPointFounded = false;
	
	// Aux
	int** cellsAux = new int* [n];
	for (size_t i = 0; i < n; i++) {
		cellsAux[i] = new int[n];
		for (size_t j = 0; j < n; j++) {
			cellsAux[i][j] = 0;
		}
	}

	while (!coordsObtained) {
		for (size_t i = 0; i < n; i++) {
			for (size_t j = 0; j < n; j++) {
				if (prevRoute[i][j] == 7 && !initialPointFounded) {
					std::tuple<int, int> actualCoord;
					coordsCounter++;
					initialPointFounded = true;
					cellsAux[i][j] = coordsCounter;
					actualCoord = std::make_tuple(i, j);
					coords.push_back(actualCoord);
				}
				if (initialPointFounded && prevRoute[i][j] != 0 && !coordsObtained) {
					getOrderedCoord(prevRoute, cellsAux, i, j, n, &coords);
				}
			}
		}
	}
	std::cout << "Coordenadas totales" << coords.size() << "\n";
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n; j++) {
			std::cout << cellsAux[i][j] << " ";
		}
		std::cout << "\n";
	}

	if (cellsAux != NULL) {
		for (int i = 0; i < n; i++) {
			delete[] cellsAux[i];
		}
		delete[] cellsAux;
	}




	return coords;
}

void PerfectRouteAutomata::getOrderedCoord(int **array, int **auxArray, int i, int j, int n, std::vector<std::tuple<int, int>> *coords) {
	std::vector<int> neighbours = getNeighbours(j, i, auxArray, n);
	bool existPrev = false;
	for (size_t k = 0; k < neighbours.size(); k++) {
		if (neighbours[k] != 0)
			existPrev = true;
	}

	if (existPrev && auxArray[i][j] == 0) {
		if (array[i][j] == 9) {	
			coordsObtained = true;
		}
		std::tuple<int, int> actualCoord;
		coordsCounter++;
		auxArray[i][j] = coordsCounter;
		actualCoord = std::make_tuple(i, j);
		(*coords).push_back(actualCoord);

	}
	
}

void PerfectRouteAutomata::prevRules(int i, int j, int** tab, int n, int** cellsAux) {
	std::vector<int> neighbours = getNeighbours(j, i, tab, n);
	bool isInitialPointAround = false, isNutrientAround = false, aroundState1 = false, aroundState2 = false;
	for (size_t i = 0; i < neighbours.size(); i++) {
		if (neighbours[i] == 3) {
			isInitialPointAround = true;
		}
		if (neighbours[i] == 6) {
			isNutrientAround = true;
		}
		if (neighbours[i] == 1) {
			aroundState1 = true;
		}
		if (neighbours[i] == 2) {
			aroundState2 = true;
		}
	}
	switch (tab[i][j]) {
		case 1:	
			if (!aroundState1)
				cellsAux[i][j] = 0;
			else
				cellsAux[i][j] = 1;
			break;
		case 2:
			if (!aroundState1)
				cellsAux[i][j] = 0;
			else
				cellsAux[i][j] = 2;
			break;
		case 3:
			cellsAux[i][j] = 7;
			break;
		case 4:
			if (!aroundState1)
				cellsAux[i][j] = 0;
			else
				cellsAux[i][j] = 4;
			break;
		case 5:
			cellsCounter++;
			if (isInitialPointAround)
				cellsAux[i][j] = 2;
			else if (isNutrientAround)
				cellsAux[i][j] = 4;
			else
				cellsAux[i][j] = 1;
			break;
		case 6:
			cellsAux[i][j] = 9;
			break;
		case 7:
			cellsAux[i][j] = 7;
			break;
		case 8:
			cellsCounter++;
			if (isInitialPointAround)
				cellsAux[i][j] = 2;
			else if(isNutrientAround)
				cellsAux[i][j] = 4;
			else
				cellsAux[i][j] = 1;
			break;
		case 9:
			cellsAux[i][j] = 9;
			break;
	}

}


std::vector<int> PerfectRouteAutomata::getNeighbours(int j, int i, int** actualArray, int size) {
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