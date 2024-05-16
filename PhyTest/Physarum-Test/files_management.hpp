#include <fstream>
#include <chrono>
#include <ctime>

#include "DensityData.hpp"
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#define FOLDER_DATA_PATH "C:\\Users\\Angel\\Documents\\OpenGL\\Physarum-Automata\\PhyTest\\Physarum-Test\\SAVE_DATA\\"
//#define FOLDER_DATA_PATH "C:\\Users\\pikmi\\Documents\\ESCOM\\TT\\Physarum-Automata\\PhyTest\\Physarum-Test\\SAVE_DATA\\"

std::string getTimeStamp() {
	std::time_t t = std::time(0);   // get time now
	std::tm* now = std::localtime(&t);
	std::string timeStamp;
	timeStamp = std::to_string(now->tm_mday) + "_" +
		std::to_string(now->tm_mon + 1) + "_" +
		std::to_string(now->tm_year + 1900) + "__" +
		std::to_string(now->tm_hour) + ";" +
		std::to_string(now->tm_min) + ";" +
		std::to_string(now->tm_sec);
	return timeStamp;
}

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
	std::string fileName = getTimeStamp();
	std::string savePathFile = FOLDER_DATA_PATH + fileName + ".txt";
	
	std::ofstream initialStateFile;
	initialStateFile.open(savePathFile);
	writeDataOnFile(&initialStateFile, cells, size);
	initialStateFile.close();
}

void writeDensityData(std::ofstream * file, std::vector<DensityData> data) {
	const int N = data.size();
	for (size_t i = 0; i < N; i++) {
		(*file) << data[i].gen << "," << data[i].st0 << "," << data[i].st1 << ","
			<< data[i].st2 << "," << data[i].st3 << "," << data[i].st4 << "," << data[i].st5 << ","
			<< data[i].st6 << "," << data[i].st7 << "," << data[i].st8 << "\n";
	}
}

void saveDensityData(std::string name, std::vector<DensityData> data) {
	std::ofstream densityDataFile;
	const std::string fileName(name + "_density.txt");
	const std::string fileDensityPath(FOLDER_DATA_PATH + fileName);
	densityDataFile.open(fileDensityPath);
	writeDensityData(&densityDataFile, data);
	densityDataFile.close();
}