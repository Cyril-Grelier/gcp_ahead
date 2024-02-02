#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Struct Graph use information from .col files to create the instance
 *
 */
struct Graph {
    /** @brief Name of the instance*/
    const std::string name;

    /** @brief Number of vertices in the graph*/
    const int nb_vertices;

    /** @brief Number of edges in the graph*/
    const int nb_edges;

    /** @brief Adjacency matrix, true if there is an edge between vertex i and vertex j*/
    const std::vector<std::vector<bool>> adjacency_matrix;

    /** @brief For each vertex, the list of its neighbors*/
    const std::vector<std::vector<int>> neighborhood;

    /** @brief For each vertex, its degree*/
    const std::vector<int> degrees;

    /** @brief List of edges*/
    const std::vector<std::pair<int, int>> edges_list;

    explicit Graph(const std::string &name_,
                   const int nb_vertices_,
                   const int nb_edges_,
                   const std::vector<std::vector<bool>> &adjacency_matrix_,
                   const std::vector<std::vector<int>> &neighborhood_,
                   const std::vector<int> &degrees_,
                   const std::vector<std::pair<int, int>> edge_lists_);

    Graph(const Graph &other) = delete;
};

struct Arc {
    int tail;
    int head;
};

bool operator==(const Arc &a1, const Arc &a2);

struct QGraph {
    /** @brief Number of arcs, can be seen as the number of vertices in the line graph*/
    const int nb_arc;
    /** @brief Directed arcs of the graph*/
    const std::vector<Arc> arcs;
    /** @brief For each vertex, the list of its neighbors in the complementary graph*/
    const std::vector<std::vector<int>> neighborhood;
    /** @brief representation of the auxiliary graph from the line graph from the directed
     * graph for the UBQP problem*/
    const std::vector<std::vector<int>> qmatrix;

    QGraph(const int nb_arc_,
           const std::vector<Arc> arcs_,
           const std::vector<std::vector<int>> &neighborhood_,
           const std::vector<std::vector<int>> &q_matrix_);
};

namespace graph_instance {
// using namespace graph_instance;
// then graph->attributes

// graph loaded from the instance
extern std::unique_ptr<const Graph> graph;
// graph for the UBQP problem
extern std::unique_ptr<const QGraph> qgraph;
} // namespace graph_instance

/**
 * @brief Load a graph from instances/reduced_gcp directory
 */
void load_graph(const std::string &instance_name);

void init_UBQP();
