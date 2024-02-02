#include "GreedyAlgorithm.hpp"

#include <algorithm>
#include <cassert>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

#include "../representation/Graph.hpp"
#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"
#include "../utils/utils.hpp"

using namespace graph_instance;
using namespace parameters_search;

GreedyAlgorithm::GreedyAlgorithm(greedy_fct_ptr greedy_function_)
    : _best_solution(), _greedy_function(greedy_function_) {
    fmt::print(parameters->output, "{}", header_csv());
}

void GreedyAlgorithm::run() {
    // init the solution
    _greedy_function(_best_solution);
    assert(_best_solution.check_solution());
    fmt::print(parameters->output, "{}", line_csv());
}

[[nodiscard]] const std::string GreedyAlgorithm::header_csv() const {
    return fmt::format("time,{}\n", Solution::header_csv);
}

[[nodiscard]] const std::string GreedyAlgorithm::line_csv() const {
    return fmt::format(
        "{},{}\n",
        parameters->elapsed_time(std::chrono::high_resolution_clock::now()),
        _best_solution.format());
}

void greedy_random(Solution &solution) {
    while (not solution.is_legal()) {
        const int vertex = solution.uncolored()[0];
        std::vector<int> possible_colors;
        for (int color = 0; color < solution.nb_colors(); ++color) {
            if (solution.nb_conflicts(vertex, color) == 0) {
                possible_colors.emplace_back(color);
            }
        }
        possible_colors.emplace_back(-1);

        solution.add_to_color(vertex, rd::choice(possible_colors));
    }
}

void greedy_constrained(Solution &solution) {
    while (not solution.is_legal()) {
        const int vertex = solution.uncolored()[0];
        std::vector<int> possible_colors;
        for (int color = 0; color < solution.nb_colors(); ++color) {
            if (solution.nb_conflicts(vertex, color) == 0) {
                possible_colors.emplace_back(color);
            }
        }
        if (possible_colors.empty()) {
            possible_colors.emplace_back(-1);
        }
        solution.add_to_color(vertex, rd::choice(possible_colors));
    }
}

void greedy_deterministic(Solution &solution) {
    while (not solution.is_legal()) {
        const int vertex = solution.uncolored()[0];
        int color_to_use = -1;
        for (int color = 0; color < solution.nb_colors(); ++color) {
            if (solution.nb_conflicts(vertex, color) == 0) {
                color_to_use = color;
                break;
            }
        }
        solution.add_to_color(vertex, color_to_use);
    }
}

void greedy_deterministic_2(Solution &solution) {
    auto perm = solution.uncolored();
    const std::vector<int> &degrees(graph->degrees);

    std::stable_sort(perm.begin(), perm.end(), [&degrees](int v1, int v2) {
        return degrees[v1] > degrees[v2];
    });

    // in case the solution is already initialized
    for (int color = 0; color < solution.nb_colors(); ++color) {
        for (auto it = perm.begin(); it != perm.end();) {
            const auto vertex = *it;
            if (solution.nb_conflicts(vertex, color) != 0) {
                ++it;
                continue;
            }
            solution.add_to_color(vertex, color);
            perm.erase(it);
        }
    }

    // start the coloring / continue it
    while (not solution.is_legal()) {
        auto it = perm.begin();
        const auto color = solution.add_to_color(*it, -1);
        perm.erase(it);
        for (it = perm.begin(); it != perm.end();) {
            const auto vertex = *it;
            if (solution.nb_conflicts(vertex, color) != 0) {
                ++it;
                continue;
            }
            solution.add_to_color(vertex, color);
            perm.erase(it);
        }
    }
}

void greedy_adaptive(Solution &solution) {
    auto perm = solution.uncolored();
    std::vector<int> degrees(graph->degrees);

    std::stable_sort(perm.begin(), perm.end(), [&degrees](int v1, int v2) {
        return degrees[v1] > degrees[v2];
    });

    // in case the solution is already initialized
    for (int color = 0; color < solution.nb_colors(); ++color) {
        for (auto it = perm.begin(); it != perm.end();) {
            const auto vertex = *it;
            if (solution.nb_conflicts(vertex, color) != 0) {
                ++it;
                continue;
            }
            solution.add_to_color(vertex, color);
            perm.erase(it);
            for (const auto neighbor : graph->neighborhood[vertex]) {
                --degrees[neighbor];
            }
        }
        std::stable_sort(perm.begin(), perm.end(), [&degrees](int v1, int v2) {
            return degrees[v1] > degrees[v2];
        });
    }

    // start the coloring / continue it
    while (not solution.is_legal()) {
        auto it = perm.begin();
        const auto color = solution.add_to_color(*it, -1);
        perm.erase(it);
        for (it = perm.begin(); it != perm.end();) {
            const auto vertex = *it;
            if (solution.nb_conflicts(vertex, color) != 0) {
                ++it;
                continue;
            }
            solution.add_to_color(vertex, color);
            perm.erase(it);
            for (const auto neighbor : graph->neighborhood[vertex]) {
                --degrees[neighbor];
            }
        }
        std::stable_sort(perm.begin(), perm.end(), [&degrees](int v1, int v2) {
            return degrees[v1] > degrees[v2];
        });
    }
}

// Struct used in conjunction with the DSatur priority queue
struct satItem {
    int sat;
    int deg;
    int vertex;
};

struct maxSat {
    bool operator()(const satItem &lhs, const satItem &rhs) const {
        // Compares two satItems sat deg, then degree, then vertex label
        if (lhs.sat > rhs.sat)
            return true;
        if (lhs.sat < rhs.sat)
            return false;
        // if we are we know that lhs.sat == rhs.sat

        if (lhs.deg > rhs.deg)
            return true;
        if (lhs.deg < rhs.deg)
            return false;
        // if we are here we know that lhs.sat == rhs.sat and lhs.deg == rhs.deg.
        // Our choice can be arbitrary
        // edit : vertices are sorted by degree so we keep this order here
        if (lhs.vertex > rhs.vertex)
            return true;
        return false;
    }
};

void greedy_DSatur(Solution &solution) {
    std::vector<int> degrees(graph->degrees);
    std::vector<std::set<int>> adjacent_colors(graph->nb_vertices, std::set<int>());
    std::set<satItem, maxSat> Q;

    // if the solution already has vertices colored, we need to update the data structures
    if (solution.nb_uncolored() != graph->nb_vertices) {
        for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
            const auto color = solution[vertex];
            if (color != -1) {
                for (const int neighbor : graph->neighborhood[vertex]) {
                    if (solution[neighbor] == -1) {
                        adjacent_colors[neighbor].insert(color);
                        --degrees[neighbor];
                    }
                }
            }
        }
    }

    // Initialise the the data structures.
    // These are a priority queue,
    // a set of colors adjacent to each uncolored vertex (initially empty) and the degree
    // d(v) of each uncolored vertex in the graph induced by uncolored vertices
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        if (solution[vertex] == -1) {
            Q.emplace(satItem{static_cast<int>(adjacent_colors[vertex].size()),
                              degrees[vertex],
                              vertex});
        }
    }

    while (!Q.empty()) {
        // Get the vertex u with highest saturation degree, breaking ties with d.
        auto maxPtr = Q.begin();
        const int vertex = (*maxPtr).vertex;
        // Remove it from the priority queue
        Q.erase(maxPtr);
        // and color it
        int color = -1;
        for (int color_ = 0; color_ < solution.nb_colors(); ++color_) {
            if (solution.nb_conflicts(vertex, color_) == 0) {
                color = color_;
                break;
            }
        }

        solution.add_to_color(vertex, color);

        // Update the saturation degrees and d-value of all uncolored neighbors; hence
        // modify their corresponding elements in the priority queue
        for (const int neighbor : graph->neighborhood[vertex]) {
            if (solution[neighbor] == -1) {
                Q.erase({static_cast<int>(adjacent_colors[neighbor].size()),
                         degrees[neighbor],
                         neighbor});
                adjacent_colors[neighbor].insert(color);
                --degrees[neighbor];
                Q.emplace(satItem{static_cast<int>(adjacent_colors[neighbor].size()),
                                  degrees[neighbor],
                                  neighbor});
            }
        }
    }
}

void update_neighborhood(const Solution &solution,
                         std::vector<int> &legal_uncolored,
                         std::vector<int> &illegal_uncolored,
                         std::vector<int> &nb_uncolored_neighbors,
                         std::vector<int> &nb_illegal_neighbors,
                         int current_vertex) {

    erase_sorted(legal_uncolored, current_vertex);
    // legal_uncolored.erase(current_vertex);

    // each uncolored neighbor of the current vertex goes in the illegal set
    // the neighbor and its neighbors are impacted by the move
    std::vector<int> impacted_vertices;
    for (const int neighbor : graph->neighborhood[current_vertex]) {
        if (solution[neighbor] == -1) {
            erase_sorted(legal_uncolored, neighbor);
            // legal_uncolored.erase(neighbor);
            // illegal_uncolored.insert(neighbor);
            if (not contains(illegal_uncolored, neighbor))
                insert_sorted(illegal_uncolored, neighbor);
            // impacted_vertices.insert(neighbor);
            if (not contains(impacted_vertices, neighbor))
                insert_sorted(impacted_vertices, neighbor);
            for (int w : graph->neighborhood[neighbor]) {
                if (solution[w] == -1) {
                    if (not contains(impacted_vertices, w))
                        insert_sorted(impacted_vertices, w);
                    // impacted_vertices.insert(w);
                }
            }
        }
    }

    // Recalculate the nb of neighbors legal and illegal for each
    // impacted vertex
    for (const int vertex : impacted_vertices) {
        nb_uncolored_neighbors[vertex] = 0;
        nb_illegal_neighbors[vertex] = 0;
        for (int neighbor : graph->neighborhood[vertex]) {
            if (solution[neighbor] == -1) {
                // if (legal_uncolored.count(neighbor) == 1)
                if (contains(legal_uncolored, neighbor))
                    nb_uncolored_neighbors[vertex]++;
                // else if (illegal_uncolored.count(neighbor) == 1)
                else if (contains(illegal_uncolored, neighbor))
                    nb_illegal_neighbors[vertex]++;
            }
        }
    }
}

void greedy_RLF(Solution &solution) {
    // vertices to color
    std::vector<int> legal_uncolored = solution.uncolored();

    // vertices that have neighbors in the current color
    std::vector<int> illegal_uncolored;

    int color = -1;

    while (not solution.is_legal()) {
        ++color;

        // nb neighbors uncolored
        std::vector<int> nb_uncolored_neighbors(graph->nb_vertices, 0);
        // nb neighbors uncolored that won't be in the current color
        std::vector<int> nb_illegal_neighbors(graph->nb_vertices, 0);

        for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
            if (solution[vertex] == color) {
                for (const auto neighbor : graph->neighborhood[vertex]) {
                    if (solution[neighbor] == -1) {
                        if (contains(legal_uncolored, neighbor)) {
                            erase_sorted(legal_uncolored, neighbor);
                        }
                        if (not contains(illegal_uncolored, neighbor)) {
                            insert_sorted(illegal_uncolored, neighbor);
                            for (const auto neighbor2 : graph->neighborhood[neighbor]) {
                                ++nb_illegal_neighbors[neighbor2];
                            }
                        }
                    }
                }
            } else if (solution[vertex] == -1) {
                for (int neighbor : graph->neighborhood[vertex]) {
                    if (contains(legal_uncolored, neighbor)) {
                        nb_uncolored_neighbors[vertex]++;
                    }
                }
            }
        }

        if (solution.nb_colors() <= color) {
            // update the nb of uncolored neighbors and select
            // the first vertex in the color, the one that has
            // the highest number of neighbors uncolored
            int first_vertex = -1;
            int nb_max_neighbors_uncolored = -1;
            for (const int vertex : legal_uncolored) {
                if (nb_uncolored_neighbors[vertex] > nb_max_neighbors_uncolored) {
                    nb_max_neighbors_uncolored = nb_uncolored_neighbors[vertex];
                    first_vertex = vertex;
                }
            }
            const auto used_color = solution.add_to_color(first_vertex, -1);
            (void)used_color;
            assert(used_color == color);

            update_neighborhood(solution,
                                legal_uncolored,
                                illegal_uncolored,
                                nb_uncolored_neighbors,
                                nb_illegal_neighbors,
                                first_vertex);
        }

        while (not legal_uncolored.empty()) {
            // select the next vertex, the one that has that has the highest number of
            // neighbors uncolored and not authorized in the current color.
            // If equality, pick the one that has the lowest number of neighbors uncolored
            // and authorized in the current color.

            int nb_max_neighbors_illegal = -1;
            int nb_min_neighbors_legal = graph->nb_vertices;
            int next_vertex = -1;
            for (int vertex : legal_uncolored) {
                if ((nb_illegal_neighbors[vertex] > nb_max_neighbors_illegal) or
                    (nb_illegal_neighbors[vertex] == nb_max_neighbors_illegal and
                     nb_uncolored_neighbors[vertex] < nb_min_neighbors_legal)) {
                    nb_max_neighbors_illegal = nb_illegal_neighbors[vertex];
                    nb_min_neighbors_legal = nb_uncolored_neighbors[vertex];
                    next_vertex = vertex;
                }
            }
            solution.add_to_color(next_vertex, color);
            update_neighborhood(solution,
                                legal_uncolored,
                                illegal_uncolored,
                                nb_uncolored_neighbors,
                                nb_illegal_neighbors,
                                next_vertex);
        }
        // legal_uncolored.swap(illegal_uncolored);
        legal_uncolored = illegal_uncolored;
        illegal_uncolored.clear();
    }
}
