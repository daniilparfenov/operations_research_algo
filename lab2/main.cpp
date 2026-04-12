#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Highs.h"

struct Graph {
    int V = 0;
    int E = 0;
    std::vector<std::vector<bool>> adj;

    void init(int numVertices) {
        V = numVertices;
        adj.assign(V, std::vector<bool>(V, 0));
    }

    void addEdge(int i, int j) {
        if ((0 <= i && i < V) && (0 <= j && j < V)) {
            adj[i][j] = 1;
            adj[j][i] = 1;
        }
    }
};

Graph parseDIMACS(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file: " + filename);
    }

    Graph graph;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == 'c')
            continue;

        std::stringstream ss(line);
        char type;
        ss >> type;

        if (type == 'p') {
            std::string col;
            int v, e;
            ss >> col >> v >> e;
            graph.init(v);
            graph.E = e;
            std::cout << "Graph loading: " << v << " vertices, " << e << " edges..." << std::endl;
        } else if (type == 'e') {
            int u, v;
            ss >> u >> v;
            graph.addEdge(u - 1, v - 1);  // in the DIMACS format, the numbering starts from 1
        }
    }
    file.close();
    return graph;
}

std::vector<std::vector<int>> findIndependentSets(const Graph& g) {
    int n = g.V;
    std::vector<std::vector<int>> independentSets;
    std::vector<std::vector<bool>> covered(n, std::vector<bool>(n, false));

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (!g.adj[i][j] && !covered[i][j]) {
                std::vector<int> independentSet;
                independentSet.push_back(i);
                independentSet.push_back(j);

                for (int k = 0; k < n; ++k) {
                    if (k == i || k == j)
                        continue;
                    bool canAdd = true;
                    for (int v : independentSet) {
                        if (g.adj[k][v]) {
                            canAdd = false;
                            break;
                        }
                    }
                    if (canAdd) {
                        independentSet.push_back(k);
                    }
                }

                // Mark all pairs from independent set as covered
                for (size_t m1 = 0; m1 < independentSet.size(); ++m1) {
                    for (size_t m2 = m1 + 1; m2 < independentSet.size(); ++m2) {
                        int u = independentSet[m1];
                        int v = independentSet[m2];
                        covered[u][v] = true;
                        covered[v][u] = true;
                    }
                }
                independentSets.push_back(independentSet);
            }
        }
    }
    return independentSets;
}

void findInitialHeuristicClique(const Graph& g, int& best_size, std::vector<int>& best_clique) {
    best_size = 0;

    for (int start_v = 0; start_v < g.V; ++start_v) {
        std::vector<int> current_clique;
        current_clique.push_back(start_v);

        std::vector<int> candidates;
        for (int i = 0; i < g.V; ++i) {
            if (g.adj[start_v][i])
                candidates.push_back(i);
        }

        while (!candidates.empty()) {
            int best_cand = -1;
            int best_deg = -1;

            // Choosing the candidate with the highest degree inside the candidate subgraph
            for (int u : candidates) {
                int deg = 0;
                for (int v : candidates) {
                    if (g.adj[u][v])
                        deg++;
                }
                if (deg > best_deg) {
                    best_deg = deg;
                    best_cand = u;
                }
            }

            current_clique.push_back(best_cand);

            std::vector<int> new_candidates;
            for (int u : candidates) {
                if (g.adj[best_cand][u])
                    new_candidates.push_back(u);
            }
            candidates = new_candidates;
        }

        if ((int)current_clique.size() > best_size) {
            best_size = current_clique.size();
            best_clique = current_clique;
        }
    }
}

bool isInteger(double val) {
    return std::abs(val - std::round(val)) < 1e-5;
}

bool combinatorialPruning(const Graph& g, int best_size, const std::vector<int>& Q, const std::vector<int>& C) {
    // Method 1: by the number of candidates
    int UB1 = Q.size() + C.size();
    if (UB1 <= best_size)
        return true;

    // Method 2: by the maximum degree of candidate vertices
    int max_deg_in_C = 0;
    for (int u : C) {
        int deg = 0;
        for (int v : C) {
            if (g.adj[u][v])
                deg++;
        }
        if (deg > max_deg_in_C)
            max_deg_in_C = deg;
    }

    int UB2 = Q.size() + (C.empty() ? 0 : max_deg_in_C + 1);
    if (UB2 <= best_size)
        return true;

    // Method 3: graph coloring
    int num_colors = 0;
    std::vector<int> color(C.size(), -1);

    for (size_t i = 0; i < C.size(); ++i) {
        int u = C[i];
        std::vector<bool> color_used(num_colors, false);

        for (size_t j = 0; j < i; ++j) {
            int v = C[j];
            if (g.adj[u][v]) {
                color_used[color[j]] = true;
            }
        }

        int c = 0;
        while (c < num_colors && color_used[c])
            c++;

        color[i] = c;
        if (c == num_colors)
            num_colors++;
    }

    int UB3 = Q.size() + num_colors;
    if (UB3 <= best_size)
        return true;

    return false;
}

void solveBnB(const Graph& g, Highs& highs, int& best_size, std::vector<int>& best_clique, const std::vector<int>& Q, const std::vector<int>& C) {
    // Pruning
    if (combinatorialPruning(g, best_size, Q, C)) {
        return;
    }

    // Solve current problem
    highs.run();
    if (highs.getModelStatus() != HighsModelStatus::kOptimal)
        return;

    double obj_val = -highs.getObjectiveValue();
    int upper_bound = (int)std::floor(obj_val + 1e-5);

    // Pruning if the current solution is worse than the best one
    if (upper_bound <= best_size)
        return;

    // Searching for a non-integer variable close to 0.5 (heuristics) for branching
    const std::vector<double>& sol = highs.getSolution().col_value;
    int branch_var = -1;
    double min_dist_to_half = 1.0;

    for (int i = 0; i < (int)sol.size(); ++i) {
        if (!isInteger(sol[i])) {
            double dist = std::abs(sol[i] - 0.5);
            if (dist < min_dist_to_half) {
                min_dist_to_half = dist;
                branch_var = i;
            }
        }
    }

    // If all variables are integers and the solution is better than the best one, update the best solution.
    if (branch_var == -1) {
        int current_size = (int)std::round(obj_val);
        if (current_size > best_size) {
            best_size = current_size;
            best_clique.clear();
            for (int i = 0; i < (int)sol.size(); ++i) {
                if (sol[i] > 0.5)
                    best_clique.push_back(i);
            }
            std::cout << ">>> NEW RECORD [LP]: " << best_size << " vertices" << std::endl;
        }
        return;
    }

    // Branching
    // Branch 1 (x = 1): including the vertex in the clique
    highs.changeColBounds(branch_var, 1.0, 1.0);
    std::vector<int> Q_next = Q;
    Q_next.push_back(branch_var);

    std::vector<int> C_next;
    C_next.reserve(C.size());
    for (int v : C) {
        // We leave only those candidates who are connected to the new vertex
        if (v != branch_var && g.adj[branch_var][v]) {
            C_next.push_back(v);
        }
    }
    solveBnB(g, highs, best_size, best_clique, Q_next, C_next);

    // Branch 1 (x = 1): excluding the vertex
    highs.changeColBounds(branch_var, 0.0, 0.0);
    std::vector<int> C_next_zero;
    C_next_zero.reserve(C.size());
    for (int v : C) {
        if (v != branch_var)
            C_next_zero.push_back(v);
    }
    solveBnB(g, highs, best_size, best_clique, Q, C_next_zero);

    // Backtracking: restoring the original constraints
    highs.changeColBounds(branch_var, 0.0, 1.0);
}

bool verifyClique(const Graph& g, const std::vector<int>& clique) {
    for (size_t i = 0; i < clique.size(); ++i) {
        for (size_t j = i + 1; j < clique.size(); ++j) {
            int u = clique[i];
            int v = clique[j];
            if (!g.adj[u][v]) {
                std::cerr << "[ERROR] Invalid clique! Vertices " << u + 1 << " and " << v + 1 << " are not connected!" << std::endl;
                return false;
            }
        }
    }
    return true;
}

void findMaxClique(const Graph& g) {
    Highs highs;
    highs.setOptionValue("output_flag", false);
    // highs.setOptionValue("time_limit", 300.0);

    HighsLp lp;
    int n = g.V;

    lp.num_col_ = n;
    lp.col_cost_.assign(n, -1.0);  // x_i -> -x_i to maximize
    lp.col_lower_.assign(n, 0.0);  // x_i >= 0
    lp.col_upper_.assign(n, 1.0);  // x_i <= 1
    highs.passModel(lp);

    // Adding constraints
    std::vector<std::vector<int>> sets = findIndependentSets(g);
    for (const auto& iset : sets) {
        int size = iset.size();
        std::vector<int> indices = iset;
        std::vector<double> values(size, 1.0);
        highs.addRow(0.0, 1.0, size, indices.data(), values.data());
    }

    // Initial clique
    int best_size = 0;
    std::vector<int> best_clique;
    findInitialHeuristicClique(g, best_size, best_clique);
    std::cout << ">>> INITIAL RECORD [Greedy]: " << best_size << " vertices" << std::endl;

    // Initializing state variables for BnB
    std::vector<int> Q_start;
    std::vector<int> C_start(g.V);
    for (int i = 0; i < g.V; ++i)
        C_start[i] = i;

    // Counting degrees
    std::vector<int> degrees(g.V, 0);
    for (int i = 0; i < g.V; i++) {
        for (int j = 0; j < g.V; j++) {
            if (g.adj[i][j])
                degrees[i]++;
        }
    }
    // Sorting candidates in descending order of degree: it improves the greedy graph coloring algorithm
    std::sort(C_start.begin(), C_start.end(), [&degrees](int a, int b) { return degrees[a] > degrees[b]; });

    // Solving the problem
    std::cout << "Starting Branch-and-Bound + Combinatorial Pruning..." << std::endl;
    solveBnB(g, highs, best_size, best_clique, Q_start, C_start);

    // Verify found clique
    std::cout << "\nVerifying solution correctness..." << std::endl;
    if (verifyClique(g, best_clique)) {
        std::cout << "[OK] The found solution is a VALID clique." << std::endl;
    } else {
        std::cout << "[FAILED] The found solution is NOT a valid clique!" << std::endl;
        return;
    }

    // Printing max clique
    std::cout << "\n==================================\n";
    std::cout << "MAXIMUM CLIQUE SIZE: " << best_size << std::endl;
    std::cout << "Vertices (DIMACS 1-based index): ";
    std::sort(best_clique.begin(), best_clique.end());
    for (int v : best_clique) {
        std::cout << v + 1 << " ";
    }
    std::cout << "\n==================================\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_graph.clq>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    try {
        Graph g = parseDIMACS(filename);
        findMaxClique(g);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
