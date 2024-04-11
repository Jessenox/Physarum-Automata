#include <fstream>
#include <chrono>
#include <ctime>
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#define FOLDER_DATA_PATH "C:\\Users\\Angel\\Documents\\OpenGL\\Physarum-Automata\\PhyTest\\Physarum-Test\\SAVE_DATA\\"

void writeDataOnFile(std::ofstream* file, int ** cells, int n) {
	(*file) << n << "\n";
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n; j++) {
			(*file) << cells[i][j] << ' ';
		}
		(*file) << "\n";
	}
}

void saveInitialState(int ** cells, int size) {
	std::time_t t = std::time(0);   // get time now
	std::tm* now = std::localtime(&t);

	std::string savePathFile = FOLDER_DATA_PATH + std::to_string(now->tm_sec) + ".txt";
	
	std::ofstream initialStateFile;
	initialStateFile.open(savePathFile);
	writeDataOnFile(&initialStateFile, cells, size);
	initialStateFile.close();
}