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
        if (line.empty() || line[0] == 'c') {
            continue;
        }

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

                // Try to expand independentSet
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

void findMaxClique(const Graph& g) {
    Highs highs;
    highs.setOptionValue("output_flag", false);

    HighsLp lp;
    int n = g.V;

    lp.num_col_ = n;
    lp.col_cost_.assign(n, -1.0);  // x_i -> -x_i to maximize
    lp.col_lower_.assign(n, 0.0);  // x_i >= 0
    lp.col_upper_.assign(n, 1.0);  // x_i <= 1
    highs.passModel(lp);

    std::vector<std::vector<int>> sets = findIndependentSets(g);
    std::cout << "Found " << sets.size() << " independent sets to construct constraints" << std::endl;
    for (auto& set : sets) {
        std::cout << "[";
        for (auto& vert : set) {
            std::cout << vert + 1 << ", ";
        }
        std::cout << "]" << std::endl;
    }

    for (const auto& iset : sets) {
        int size = iset.size();
        std::vector<int> indices = iset;
        std::vector<double> values(size, 1.0);
        highs.addRow(0.0, 1.0, size, indices.data(), values.data());  // 0 <= sum(x_i) <= 1 where i is a vertix from an independent set
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_graph>" << std::endl;
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
