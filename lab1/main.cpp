#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "Highs.h"

std::vector<std::vector<double>> readCSV(const std::string& filename) {
    std::vector<std::vector<double>> matrix;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return matrix;  // Return empty matrix
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;  // Skip empty lines

        std::vector<double> row;
        std::stringstream ss(line);
        std::string val;

        while (std::getline(ss, val, ';')) {
            try {
                row.push_back(std::stod(val));
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

void printMatrix(const std::vector<std::vector<double>>& matrix) {
    for (const auto& row : matrix) {
        std::cout << '[';
        for (const auto& el : row) {
            std::cout << std::setw(8) << el << " ";
        }
        std::cout << "]\n";
    }
}

int findLowerGameVal(const std::vector<std::vector<double>>& matrix) {
    int lowerGameVal = std::numeric_limits<int>::min();

    for (const auto& row : matrix) {
        int minInARow = *std::min_element(row.begin(), row.end());
        lowerGameVal = std::max(lowerGameVal, minInARow);
    }

    return lowerGameVal;
}

void solveGame(const std::string& filename) {
    auto matrix = readCSV(filename);
    if (matrix.empty()) {
        std::cout << "Error: empty matrix is given" << std::endl;
        return;
    }

    int lowerGameVal = findLowerGameVal(matrix);
    int shift = 0;
    if (lowerGameVal <= 0) {
        shift = std::abs(lowerGameVal);
    }

    int m = matrix.size();
    int n = matrix[0].size();

    HighsModel model;
    model.lp_.sense_ = ObjSense::kMinimize;
    model.lp_.num_col_ = m;
    model.lp_.num_row_ = n;

    // Objective function: x_0 + x_1 + ... + x_m-1 -> min
    model.lp_.col_cost_.assign(m, 1.0);

    // Bounds on the variables: x_i >= 0
    model.lp_.col_lower_.assign(m, 0.0);
    model.lp_.col_upper_.assign(m, kHighsInf);

    // Bounds on constraints: A[j][0]*x_0 + A[j][1]*x_1 + ... + A[j][n-1]*x_n-1 >= 1
    model.lp_.row_lower_.assign(n, 1.0);
    model.lp_.row_upper_.assign(n, kHighsInf);

    // Constraint matrix
    model.lp_.a_matrix_.format_ = MatrixFormat::kColwise;
    model.lp_.a_matrix_.start_.clear();

    // The start of 1st columnt is 0
    model.lp_.a_matrix_.start_.push_back(0);
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            // Index of an element in a row
            model.lp_.a_matrix_.index_.push_back(j);

            // Shifted value of matrix element
            double val = matrix[i][j] + shift;
            model.lp_.a_matrix_.value_.push_back(val);
        }
        // The end of cur column
        model.lp_.a_matrix_.start_.push_back(model.lp_.a_matrix_.index_.size());
    }

    // Solve the problem
    Highs highs;
    highs.setOptionValue("output_flag", false);
    highs.passModel(model);
    highs.run();

    // Checking whether the optimization was successful
    if (highs.getModelStatus() != HighsModelStatus::kOptimal) {
        std::cout << "Optimization failed" << std::endl;
        return;
    }

    // Print the result

    // Z = 1 / v', v' - shifted game value
    double objectiveValue = highs.getInfo().objective_function_value;

    // v' = 1 / S
    double v_shifted = 1.0 / objectiveValue;

    // v = v' - shift
    double gameValue = v_shifted - shift;

    std::cout << "Game Value: " << gameValue << std::endl;

    // Player 1 strategy: p_i = x_i * v'
    const std::vector<double>& primal_solution = highs.getSolution().col_value;
    std::cout << "Player 1 Probabilities: [ ";
    for (double val : primal_solution) {
        std::cout << (val * v_shifted) << " ";
    }
    std::cout << "]" << std::endl;

    // Player 2 strategy - the dual solution
    const std::vector<double>& dual_solution = highs.getSolution().row_dual;
    std::cout << "Player 2 Probabilities: [ ";
    for (double val : dual_solution) {
        std::cout << (val * v_shifted) << " ";
    }
    std::cout << "]" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_csv>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    solveGame(filename);

    return 0;
}
