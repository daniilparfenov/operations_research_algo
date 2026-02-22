#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Highs.h"

std::vector<std::vector<int>> readCSV(const std::string& filename) {
    std::vector<std::vector<int>> matrix;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return matrix;  // Return empty matrix
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;  // Skip empty lines

        std::vector<int> row;
        std::stringstream ss(line);
        std::string val;

        while (std::getline(ss, val, ';')) {
            try {
                row.push_back(std::stoi(val));
            } catch (const std::exception& e) {
                std::cerr << "Error: Could not convert to int matrix element " << val << std::endl;
            }
        }

        if (!row.empty()) {
            matrix.push_back(row);
        }
    }

    file.close();
    return matrix;
}

void printMatrix(const std::vector<std::vector<int>>& matrix) {
    for (const auto& row : matrix) {
        std::cout << '[';
        for (const auto& el : row) {
            std::cout << std::setw(8) << el << " ";
        }
        std::cout << "]\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_csv>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    auto matrix = readCSV(filename);
    printMatrix(matrix);

    return 0;
}
