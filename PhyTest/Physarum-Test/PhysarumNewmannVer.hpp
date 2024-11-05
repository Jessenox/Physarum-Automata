class PhysarumNewmannVer {
	public:
		PhysarumNewmannVer(int**, int**, int**, int, int, int, int);
		~PhysarumNewmannVer();
		std::vector<int> getNeighboursNewmann(int, int, int**);
		void physarumTransitionConditions(int, int, int, std::vector<int>);
	private:
		int setCurrentDirection();
		bool isOnMooreOffsetNewmann(std::vector<int>, int);
		bool isOnCurrentDirection(int, int, std::vector<int>);
		bool findState(int, std::vector<int>);
	public:
		int** cellsAux;
		int nutrientFounded = 0;
		int nutrientNotFounded = 0;
		int physarumActualCells = 0;
		int** cellsMemory;
	private:
		int** cells;
		int size = 0;
};

PhysarumNewmannVer::PhysarumNewmannVer(int ** base_array, int** aux_array, int** memory_array, int n, int nut, int notNut, int phycells) {
	cells = base_array;
	cellsAux = aux_array;
	cellsMemory = memory_array;
	size = n;
	nutrientFounded = nut;
	nutrientNotFounded = notNut;
	physarumActualCells = phycells;
}

PhysarumNewmannVer::~PhysarumNewmannVer() {
}

bool PhysarumNewmannVer::isOnCurrentDirection(int direction, int state, std::vector<int> neighboursData) {
	if (neighboursData[direction] == state) {
		return true;
	}
	else {
		return false;
	}
}


void PhysarumNewmannVer::physarumTransitionConditions(int i, int j, int cell, std::vector<int> neighboursData) {
	int sizeArray = neighboursData.size();
	int currentCellDirection = setCurrentDirection();
	std::vector<int> neighboursDirections = getNeighboursNewmann(j, i, cellsMemory);

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
		break;
	case 1:
		if (onState5 || onState6) {
			cellsAux[i][j] = 6;
		}
		nutrientNotFounded++;
		
		break;
	case 2:
		
		break;
	case 3:
		
		break;
	case 4:
		if ((((isOnCurrentDirection(currentCellDirection, 3, neighboursData) ||
			isOnCurrentDirection(currentCellDirection, 5, neighboursData) ||
			isOnCurrentDirection(currentCellDirection, 6, neighboursData)) &&
			cellsMemory[i][j] == 0 && !onState0 && !onState7))) {
			cellsAux[i][j] = 5;
			cellsMemory[i][j] = currentCellDirection + 1;
		}
		
		break;
	case 5:
		if (!isOnMooreOffsetNewmann(neighboursDirections, 5) &&
			!isOnMooreOffsetNewmann(neighboursDirections, 8) &&
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

int PhysarumNewmannVer::setCurrentDirection() {
	RandomRange rnd;
	return rnd.getRandom(0, 3);
}

bool PhysarumNewmannVer::findState(int state, std::vector<int> neighboursData) {
	for (size_t i = 0; i < neighboursData.size(); i++)
		if (neighboursData[i] == state)
			return true;

	return false;
}

bool PhysarumNewmannVer::isOnMooreOffsetNewmann(std::vector<int> neighboursDirections, int state) {
	for (size_t i = 2, j = 0; j < neighboursDirections.size(); i++, j++) {
		if (i > 3) {
			i = 0;
		}
		if (neighboursDirections[i] - 1 == j) {
			return true;
		}
	}
	return false;
}

std::vector<int> PhysarumNewmannVer::getNeighboursNewmann(int j, int i, int** actualArray) {
	std::vector<int> data;
	if (i == 0) {
		data.push_back(actualArray[size - 1][j]);
		if (j == size - 1) {
			data.push_back(actualArray[i][0]);
		}
		else {
			data.push_back(actualArray[i][j + 1]);
		}
		data.push_back(actualArray[i + 1][j]);
		if (j == 0) {
			data.push_back(actualArray[i][size - 1]);
		}
		else {
			data.push_back(actualArray[i][j - 1]);
		}
	}
	else if (i == size - 1) {
		data.push_back(actualArray[i - 1][j]);
		if (j == size - 1) {
			data.push_back(actualArray[i][0]);
		}
		else {
			data.push_back(actualArray[i][j + 1]);
		}
		data.push_back(actualArray[0][j]);
		if (j == 0) {
			data.push_back(actualArray[i][size - 1]);
		}
		else {
			data.push_back(actualArray[i][j - 1]);
		}
	}
	else {
		data.push_back(actualArray[i - 1][j]);
		if (j == size - 1) {
			data.push_back(actualArray[i][0]);
		}
		else {
			data.push_back(actualArray[i][j + 1]);
		}
		data.push_back(actualArray[i + 1][j]);
		if (j == 0) {
			data.push_back(actualArray[i][size - 1]);
		}
		else {
			data.push_back(actualArray[i][j - 1]);
		}
	}
	return data;
}


