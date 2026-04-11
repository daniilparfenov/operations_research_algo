#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

struct Graph {
    int V = 0;
    int E = 0;
    std::vector<std::vector<bool>> adj;

    void init(int numVertices)
    {
        V = numVertices;
        adj.assign(V, std::vector<bool>(V, 0));
    }

    void addEdge(int i, int j)
    {
        if ((0 <= i && i < V) && (0 <= j && j <= V))
        {
            adj[i][j] = 1;
            adj[j][i] = 1;
        }
    }
};

Graph parseDIMACS(const std::string& filename)
{
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
        }
        else if (type == 'e') {
            int u, v;
            ss >> u >> v;
            graph.addEdge(u - 1, v - 1); // in the DIMACS format, the numbering starts from 1
        }
    }

    file.close();
    return graph;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_graph>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    try
    {
        parseDIMACS(filename);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }


    return 0;
}
