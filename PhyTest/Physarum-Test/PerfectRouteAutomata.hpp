#include <vector>
#include <algorithm> 
#include <iostream>

class PerfectRouteAutomata {
public:
	void getPrevRoute(int**, int);
private:
	void evaluateAutomata(int**, int);
	void prevRules(int, int, int**, int, int**);
	std::vector<int> getNeighbours(int j, int i, int**, int );
	std::vector<std::tuple<int, int>> getOrderedCoords(int **, int);
	std::vector<std::tuple<int, int>> getAllPhysarumCoords(int **, int);
	std::vector<std::tuple<int, int, int>> getManhattanDistance(std::vector<std::tuple<int, int>> );
	std::vector<std::tuple<int, int>> getAutomataCoords(int** , int );
public:
	std::vector<std::tuple<int, int>> myCoords;
private:
	std::tuple<int, int> initialPoint;
	std::tuple<int, int> finalPoint;
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
	
	
	myCoords = getAutomataCoords(result, n);
	
	for (size_t i = 0; i < myCoords.size(); i++) {
		std::cout << std::get<0>(myCoords[i]) << ", " << std::get<1>(myCoords[i]) << "\n";
	}
	
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
			if (tab[i][j] == 9) {
				finalPoint = std::make_tuple(j, i);
			}
			if (tab[i][j] == 7) {
				initialPoint = std::make_tuple(j, i);
			}
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

std::vector<std::tuple<int, int>> PerfectRouteAutomata::getAllPhysarumCoords(int** tab, int n) {
	std::vector<std::tuple<int, int>> coords;
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n; j++) {
			std::tuple<int, int> actualCoord;
			// x , y => j , i
			if (tab[i][j] != 0 && tab[i][j] != 9 && tab[i][j] != 7) {
				actualCoord = std::make_tuple(j, i);
				coords.push_back(actualCoord);
			}
		}
	}
	return coords;
}

bool sorting(const std::tuple<int, int, int> &a, const std::tuple<int, int, int> &b) {
	return (std::get<2>(a) < std::get<2>(b));
}

std::vector<std::tuple<int, int, int>> PerfectRouteAutomata::getManhattanDistance(std::vector<std::tuple<int, int>> coords) {
	std::vector<std::tuple<int, int, int>> coordsWithDistance;
	coordsWithDistance.push_back(std::make_tuple(std::get<0>(initialPoint), std::get<1>(initialPoint), 0));
	for (size_t i = 0; i < coords.size(); i++) {
		coordsWithDistance.push_back(std::make_tuple(std::get<0>(coords[i]), std::get<1>(coords[i]),
		(abs(std::get<0>(coords[i]) - std::get<0>(initialPoint)) + abs(std::get<1>(coords[i]) - std::get<1>(initialPoint)))));
	}
	return coordsWithDistance;
}

std::vector<std::tuple<int, int>> PerfectRouteAutomata::getOrderedCoords(int** prevRoute, int n) {
	std::vector<std::tuple<int, int>> coords = getAllPhysarumCoords(prevRoute, n); ;
	
	std::vector<std::tuple<int, int, int>> coordsSized = getManhattanDistance(coords);
	coords.clear();
	// Sort coords
	std::sort(coordsSized.begin(), coordsSized.end(), sorting);
	// Set all coords
	for (size_t i = 0; i < coordsSized.size(); i++) {
		//std::cout << std::get<0>(coordsSized[i]) << ", " << std::get<1>(coordsSized[i]) << ": " << std::get<2>(coordsSized[i]) << "\n";
		coords.push_back(std::make_tuple(std::get<0>(coordsSized[i]), std::get<1>(coordsSized[i])));
	}
	//
	coordsSized.clear();
	coords.push_back(std::make_tuple(std::get<0>(finalPoint), std::get<1>(finalPoint)));
	return coords;
}

std::vector<std::tuple<int, int>> PerfectRouteAutomata::getAutomataCoords(int** tab, int n) {
	std::vector<std::tuple<int, int>> coords;

	int** cellsAux = new int* [n];
	for (size_t i = 0; i < n; i++) {
		cellsAux[i] = new int[n];
		for (size_t j = 0; j < n; j++) {
			cellsAux[i][j] = 0;
		}
	}

	int count = 0;
	bool end = false;

	while (!end) {
		for (size_t i = 0; i < n; i++) {
			for (size_t j = 0; j < n; j++) {
				// get neigbours if exists around
				std::vector<int> neighbours = getNeighbours(j, i, cellsAux, n);
				bool isAround = false;
				for (size_t k = 0; k < neighbours.size(); k++) {
					if ((neighbours[k] - count) < 9)
						isAround = true;
				}
				switch (tab[i][j]) {
					case 7:
						if (count == 0) {
							count++;
							coords.push_back(std::make_tuple(j, i));
							cellsAux[i][j] = count;
						}
					break;
					case 9:
						if (isAround) {
							coords.push_back(std::make_tuple(j, i));
							end = true;
						}
					break;
					default:
						if (isAround && tab[i][j] != 0) {
							coords.push_back(std::make_tuple(j, i));
							cellsAux[i][j] = count;
							count++;
						}
					break;
				}
			}
		}
	}
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