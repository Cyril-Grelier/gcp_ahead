#include "crossover.hpp"

#include <cassert>
#include <cmath>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

#include "../representation/Graph.hpp"
#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"

using namespace graph_instance;
using namespace parameters_search;

Crossover::Crossover(const crossover_ptr function_, const ParamCrossover &parameters_)
    : function(function_), param(parameters_) {
}

void Crossover::run(const Solution &parent1,
                    const Solution &parent2,
                    Solution &child) const {
    function(parent1, parent2, child, param);
}

void no_crossover(const Solution &parent1,
                  const Solution &parent2,
                  Solution &child,
                  const ParamCrossover &param) {
    (void)parent2; // to remove warning "unused parameter"
    (void)param;   // to remove warning "unused parameter"
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        child.add_to_color(vertex, parent1[vertex]);
    }
}

void gpx(const Solution &parent1,
         const Solution &parent2,
         Solution &child,
         const ParamCrossover &param) {
    assert(parent1.nb_colors() == parent2.nb_colors());
    assert(parent1.nb_uncolored() == 0);
    assert(parent2.nb_uncolored() == 0);
    std::vector<int> colors_sizes_p1(parent1.nb_colors(), 0);
    std::vector<std::vector<int>> color_groups_p1(parent1.nb_colors());
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int color = parent1[vertex];
        if (color != -1) {
            color_groups_p1[color].emplace_back(vertex);
            ++colors_sizes_p1[color];
        }
    }
    std::vector<int> colors_sizes_p2(parent1.nb_colors(), 0);
    std::vector<std::vector<int>> color_groups_p2(parent1.nb_colors());
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int color = parent2[vertex];
        if (color != -1) {
            color_groups_p2[color].emplace_back(vertex);
            ++colors_sizes_p2[color];
        }
    }
    int add_color = 0;
    for (int color = 0; color < parent1.nb_colors(); color++) {
        const auto &parent = (color % (param.colors_p1 + 1) < param.colors_p1)
                                 ? color_groups_p1
                                 : color_groups_p2;
        const auto &colors_sizes = (color % (param.colors_p1 + 1) < param.colors_p1)
                                       ? colors_sizes_p1
                                       : colors_sizes_p2;

        // index/color of max number of vertex in colors
        const int max_color = static_cast<int>(
            std::distance(colors_sizes.begin(),
                          std::max_element(colors_sizes.begin(), colors_sizes.end())));

        if (colors_sizes[max_color] != 0) {
            int current_color = -1;
            for (const auto &vertex : parent[max_color]) {
                if (child[vertex] == -1) {
                    current_color = child.add_to_color(vertex, current_color);
                    --colors_sizes_p1[parent1[vertex]];
                    --colors_sizes_p2[parent2[vertex]];
                }
            }
        } else {
            // it can happen that colors are empty if conflicts are located in different
            // places in each parents
            ++add_color;
        }
    }
    // open new colors for vertices in conflict
    while (add_color > 0) {
        const int vertex = child.conflicting_vertices()[0];
        child.delete_from_color(vertex);
        child.add_to_color(vertex, -1);
        --add_color;
    }
    std::vector<int> to_color_illegal;
    // color uncolored vertices in the first available color
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        if (child[vertex] != -1) {
            continue;
        }
        for (int color = 0; color < child.nb_colors(); ++color) {
            if (child.nb_conflicts(vertex, color) == 0) {
                child.add_to_color(vertex, color);
                break;
            }
        }
        if (child[vertex] == -1) {
            to_color_illegal.emplace_back(vertex);
        }
    }
    for (const int vertex : to_color_illegal) {
        int min_conflicts = graph->nb_vertices;
        std::vector<int> best_colors;
        for (int color = 0; color < child.nb_colors(); ++color) {
            const int nb_conflicts = child.nb_conflicts(vertex, color);
            if (nb_conflicts > min_conflicts) {
                continue;
            }
            if (nb_conflicts < min_conflicts) {
                best_colors.clear();
                min_conflicts = nb_conflicts;
            }
            best_colors.emplace_back(color);
        }
        child.add_to_color(vertex, rd::choice(best_colors));
    }
    assert(child.nb_colors() == parent1.nb_colors());
}

void partial_best_gpx(const Solution &parent1,
                      const Solution &parent2,
                      Solution &child,
                      const ParamCrossover &param) {
    const int nb_colors = parent1.nb_colors();
    assert(nb_colors == parent2.nb_colors());
    assert(parent1.nb_uncolored() == 0);
    assert(parent2.nb_uncolored() == 0);

    // pick n colors in p1 then alternate between p1 and p2
    std::vector<int> colors_sizes_p1(nb_colors, 0);
    std::vector<std::vector<int>> color_groups_p1(nb_colors);
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int color = parent1[vertex];
        if (color != -1) {
            color_groups_p1[color].emplace_back(vertex);
            ++colors_sizes_p1[color];
        }
    }
    std::vector<int> colors_sizes_p2(nb_colors, 0);
    std::vector<std::vector<int>> color_groups_p2(nb_colors);
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int color = parent2[vertex];
        if (color != -1) {
            color_groups_p2[color].emplace_back(vertex);
            ++colors_sizes_p2[color];
        }
    }
    // pick the n largest colors in p1 and add them to the child
    int n = param.percentage_p1 * graph->nb_vertices / 100;
    for (int i = 0; i < n; ++i) {
        const int max_color = static_cast<int>(std::distance(
            colors_sizes_p1.begin(),
            std::max_element(colors_sizes_p1.begin(), colors_sizes_p1.end())));
        int current_color = -1;
        for (const auto &vertex : color_groups_p1[max_color]) {
            if (child[vertex] == -1) {
                current_color = child.add_to_color(vertex, current_color);
                --colors_sizes_p1[parent1[vertex]];
                --colors_sizes_p2[parent2[vertex]];
            }
        }
    }
    // alternate between p1 and p2
    for (int color = n; color < nb_colors; ++color) {
        const auto &parent = (color % 2 == 0) ? color_groups_p1 : color_groups_p2;
        const auto &colors_sizes = (color % 2 == 0) ? colors_sizes_p1 : colors_sizes_p2;

        // index/color of max number of vertex in colors
        const int max_color = static_cast<int>(
            std::distance(colors_sizes.begin(),
                          std::max_element(colors_sizes.begin(), colors_sizes.end())));

        if (colors_sizes[max_color] != 0) {
            int current_color = -1;
            for (const auto &vertex : parent[max_color]) {
                if (child[vertex] == -1) {
                    current_color = child.add_to_color(vertex, current_color);
                    --colors_sizes_p1[parent1[vertex]];
                    --colors_sizes_p2[parent2[vertex]];
                }
            }
        }
    }
    // color uncolored vertices in the child
    // while increasing the least the number of conflicts
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        if (child[vertex] != -1) {
            continue;
        }
        int min_conflicts = graph->nb_vertices;
        std::vector<int> best_colors;
        for (int color = 0; color < child.nb_colors(); ++color) {
            const int nb_conflicts = child.nb_conflicts(vertex, color);
            if (nb_conflicts > min_conflicts) {
                continue;
            }
            if (nb_conflicts < min_conflicts) {
                best_colors.clear();
                min_conflicts = nb_conflicts;
            }
            best_colors.emplace_back(color);
        }
        child.add_to_color(vertex, rd::choice(best_colors));
    }
}

void partial_random_gpx(const Solution &parent1,
                        const Solution &parent2,
                        Solution &child,
                        const ParamCrossover &param) {
    const int nb_colors = parent1.nb_colors();
    assert(nb_colors == parent2.nb_colors());
    assert(parent1.nb_uncolored() == 0);
    assert(parent2.nb_uncolored() == 0);

    // pick n colors in p1 then alternate between p1 and p2
    std::vector<int> colors_sizes_p1(nb_colors, 0);
    std::vector<std::vector<int>> color_groups_p1(nb_colors);
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int color = parent1[vertex];
        if (color != -1) {
            color_groups_p1[color].emplace_back(vertex);
            ++colors_sizes_p1[color];
        }
    }
    std::vector<int> colors_sizes_p2(nb_colors, 0);
    std::vector<std::vector<int>> color_groups_p2(nb_colors);
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int color = parent2[vertex];
        if (color != -1) {
            color_groups_p2[color].emplace_back(vertex);
            ++colors_sizes_p2[color];
        }
    }
    // pick n random colors in p1 and add them to the child
    int n = param.percentage_p1 * parameters->nb_colors / 100;
    for (int i = 0; i < n; ++i) {
        // pick a random color non empty in p1
        std::vector<int> non_empty_colors;
        for (int color = 0; color < nb_colors; ++color) {
            if (colors_sizes_p1[color] > 0) {
                non_empty_colors.emplace_back(color);
            }
        }
        const int max_color = rd::choice(non_empty_colors);
        int current_color = -1;
        for (const auto &vertex : color_groups_p1[max_color]) {
            if (child[vertex] == -1) {
                current_color = child.add_to_color(vertex, current_color);
                --colors_sizes_p1[parent1[vertex]];
                --colors_sizes_p2[parent2[vertex]];
            }
        }
    }
    // alternate between p1 and p2
    for (int color = n; color < nb_colors; ++color) {
        const auto &parent = (color % 2 == 0) ? color_groups_p1 : color_groups_p2;
        const auto &colors_sizes = (color % 2 == 0) ? colors_sizes_p1 : colors_sizes_p2;

        // index/color of max number of vertex in colors
        const int max_color = static_cast<int>(
            std::distance(colors_sizes.begin(),
                          std::max_element(colors_sizes.begin(), colors_sizes.end())));

        if (colors_sizes[max_color] != 0) {
            int current_color = -1;
            for (const auto &vertex : parent[max_color]) {
                if (child[vertex] == -1) {
                    current_color = child.add_to_color(vertex, current_color);
                    --colors_sizes_p1[parent1[vertex]];
                    --colors_sizes_p2[parent2[vertex]];
                }
            }
        }
    }
    // color uncolored vertices in the child
    // while increasing the least the number of conflicts
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        if (child[vertex] != -1) {
            continue;
        }
        int min_conflicts = graph->nb_vertices;
        std::vector<int> best_colors;
        for (int color = 0; color < child.nb_colors(); ++color) {
            const int nb_conflicts = child.nb_conflicts(vertex, color);
            if (nb_conflicts > min_conflicts) {
                continue;
            }
            if (nb_conflicts < min_conflicts) {
                best_colors.clear();
                min_conflicts = nb_conflicts;
            }
            best_colors.emplace_back(color);
        }
        child.add_to_color(vertex, rd::choice(best_colors));
    }
}
