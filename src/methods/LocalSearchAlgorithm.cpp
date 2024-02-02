#include "LocalSearchAlgorithm.hpp"

#include <algorithm>
#include <cassert>
#include <map>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

#include "../representation/Graph.hpp"
#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"
#include "../utils/utils.hpp"
#include "GreedyAlgorithm.hpp"

using namespace graph_instance;
using namespace parameters_search;

LocalSearch::LocalSearch(const local_search_ptr function_, const ParamLS &parameters_)
    : function(function_), param(parameters_) {
}

std::optional<Solution> LocalSearch::run(Solution &solution) const {
    return function(solution, param);
}

LocalSearchAlgorithm::LocalSearchAlgorithm(greedy_fct_ptr greedy_function_,
                                           const LocalSearch &local_search_)
    : _best_solution(), _greedy_function(greedy_function_), _local_search(local_search_) {
    fmt::print(parameters->output, "{}", header_csv());
}

void LocalSearchAlgorithm::run() {
    _greedy_function(_best_solution);
    assert(_best_solution.check_solution());
    fmt::print(parameters->output, "{}", line_csv());

    _local_search.run(_best_solution);
}

[[nodiscard]] const std::string LocalSearchAlgorithm::header_csv() const {
    return fmt::format("turn,time,{}\n", Solution::header_csv);
}

[[nodiscard]] const std::string LocalSearchAlgorithm::line_csv() const {
    return fmt::format(
        "0,{},{}\n",
        parameters->elapsed_time(std::chrono::high_resolution_clock::now()),
        _best_solution.format());
}

std::optional<Solution> partial_col(Solution &best_solution, const ParamLS &param) {
    const auto max_time =
        std::chrono::high_resolution_clock::now() + std::chrono::seconds(param.max_time);

    int64_t best_time = 0;
    int64_t best_turn = 0;
    int64_t best_legal_time = 0;
    int64_t best_legal_turn = 0;
    std::optional<Solution> best_legal_solution = std::nullopt;
    if (best_solution.is_legal()) {
        best_legal_solution = best_solution;
    }

    std::uniform_int_distribution<long> distribution_tabu(param.random_min,
                                                          param.random_max);

    if (best_solution.penalty() != 0) {
        best_solution.remove_penalty();
    }

    if (parameters->use_target and best_solution.nb_colors() > parameters->nb_colors) {
        // remove all excess colors
        best_solution =
            best_solution.reduce_nb_colors_partial_legal(parameters->nb_colors);
        assert(best_solution.check_solution());
    }

    Solution solution = best_solution;
    long turn = 0;

    while (not parameters->time_limit_reached_sub_method(max_time) and
           turn < param.max_iterations and
           not(best_solution.is_legal() and
               best_solution.nb_colors() == parameters->nb_colors)) {

        if (solution.is_legal() and not parameters->use_target) {
            solution = solution.reduce_nb_colors_partial_legal(solution.nb_colors() - 1);
            best_solution = solution;
            if (param.verbose) {
                print_result_ls(best_time, best_solution, turn);
            }
            assert(solution.check_solution());
        }

        int best_found = solution.nb_uncolored();
        std::vector<std::vector<long>> tabu_matrix(
            graph->nb_vertices, std::vector<long>(solution.nb_colors(), 0));

        std::uniform_int_distribution<int> distribution_colors(0,
                                                               solution.nb_colors() - 1);
        turn = 0;
        while (not parameters->time_limit_reached_sub_method(max_time) and
               turn < param.max_iterations and not best_solution.is_legal()) {

            ++turn;

            int best_current = std::numeric_limits<int>::max();
            std::vector<Coloration> best_colorations;

            for (const int vertex : solution.uncolored()) {
                for (int color = 0; color < solution.nb_colors(); ++color) {
                    const int nb_conflicts = solution.nb_conflicts(vertex, color);
                    if (nb_conflicts > best_current) {
                        continue;
                    }
                    const bool is_move_tabu = tabu_matrix[vertex][color] >= turn;
                    const bool is_improving =
                        nb_conflicts == 0 and solution.nb_uncolored() <= best_found;
                    if (is_move_tabu and not is_improving) {
                        continue;
                    }

                    if (nb_conflicts < best_current) {
                        best_current = nb_conflicts;
                        best_colorations.clear();
                    }
                    best_colorations.emplace_back(Coloration{vertex, color});
                }
            }
            // If no move, pick a random one
            if (best_colorations.empty()) {
                const int vertex = rd::choice(solution.uncolored());
                const int color = distribution_colors(rd::generator);
                best_colorations.emplace_back(Coloration{vertex, color});
            }

            const auto [vertex, color] = rd::choice(best_colorations);
            solution.grenade_move(vertex, color);

            // Block neighbor of the best move from coming to the color
            for (const int neighbor : graph->neighborhood[vertex]) {
                long t_tenure =
                    static_cast<long>(param.alpha *
                                      static_cast<double>(solution.nb_uncolored())) +
                    distribution_tabu(rd::generator);
                tabu_matrix[neighbor][color] = turn + t_tenure;
            }

            assert(solution.check_solution());

            if (solution.nb_uncolored() < best_found) {
                best_found = solution.nb_uncolored();
                best_solution = solution;
                best_time =
                    parameters->elapsed_time(std::chrono::high_resolution_clock::now());
                best_turn = turn;
                if (param.verbose) {
                    print_result_ls(best_time, best_solution, turn);
                }
            }
        }
        if (solution.is_legal()) {
            best_legal_solution = solution;
            best_legal_time = best_time;
            best_legal_turn = best_turn;
        }
    }
    if (param.verbose) {
        print_result_ls(best_time, best_solution, best_turn);
        if (best_legal_solution) {
            print_result_ls(best_legal_time, best_legal_solution, best_legal_turn);
        }
    }
    if (best_solution.nb_uncolored() != 0) {
        best_solution.color_uncolored();
    }
    assert(best_solution.check_solution());

    return best_legal_solution;
}

std::optional<Solution> partial_col_optimized(Solution &best_solution,
                                              const ParamLS &param) {
    const auto max_time =
        std::chrono::high_resolution_clock::now() + std::chrono::seconds(param.max_time);

    int64_t best_time = 0;
    int64_t best_turn = 0;
    int64_t best_legal_time = 0;
    int64_t best_legal_turn = 0;
    std::optional<Solution> best_legal_solution = std::nullopt;
    if (best_solution.is_legal()) {
        best_legal_solution = best_solution;
    }

    std::uniform_int_distribution<long> distribution_tabu(param.random_min,
                                                          param.random_max);

    if (best_solution.penalty() != 0) {
        best_solution.remove_penalty();
    }

    if (parameters->use_target and best_solution.nb_colors() > parameters->nb_colors) {
        // remove all excess colors
        best_solution =
            best_solution.reduce_nb_colors_partial_legal(parameters->nb_colors);
        assert(best_solution.check_solution());
    }

    Solution solution = best_solution;
    long turn = 0;

    while (not parameters->time_limit_reached_sub_method(max_time) and
           turn < param.max_iterations and
           not(best_solution.is_legal() and
               best_solution.nb_colors() == parameters->nb_colors)) {

        if (solution.is_legal() and not parameters->use_target) {
            solution = solution.reduce_nb_colors_partial_legal(solution.nb_colors() - 1);
            best_solution = solution;
            if (param.verbose) {
                print_result_ls(best_time, best_solution, turn);
            }
            assert(solution.check_solution());
        }

        int best_found = solution.nb_uncolored();
        std::vector<std::vector<long>> tabu_matrix(
            graph->nb_vertices, std::vector<long>(solution.nb_colors(), 0));

        std::uniform_int_distribution<int> distribution_colors(0,
                                                               solution.nb_colors() - 1);
        solution.init_deltas_optimized();
        assert(solution.check_solution());

        turn = 0;
        while (not parameters->time_limit_reached_sub_method(max_time) and
               turn < param.max_iterations and not best_solution.is_legal()) {

            ++turn;

            int best_current = std::numeric_limits<int>::max();
            std::vector<Coloration> best_colorations;

            for (const int vertex : solution.uncolored()) {
                const int nb_conflict = solution.best_delta(vertex);
                if (nb_conflict > best_current) {
                    continue;
                }

                const int current_color = solution[vertex];
                bool added = false;

                for (const auto &color : solution.best_improve_colors(vertex)) {
                    if (color == current_color) {
                        continue;
                    }

                    const bool is_move_tabu = tabu_matrix[vertex][color] >= turn;
                    const bool is_improving =
                        nb_conflict == 0 and solution.nb_uncolored() <= best_found;
                    if (is_move_tabu and not is_improving) {
                        continue;
                    }

                    added = true;
                    if (nb_conflict < best_current) {
                        best_current = nb_conflict;
                        best_colorations.clear();
                    }
                    best_colorations.emplace_back(Coloration{vertex, color});
                }

                if (added or (nb_conflict >= best_current)) {
                    continue;
                }

                for (int color = 0; color < solution.nb_colors(); ++color) {
                    const int nb_conflicts = solution.nb_conflicts(vertex, color);
                    if (nb_conflicts > best_current) {
                        continue;
                    }
                    const bool is_move_tabu = tabu_matrix[vertex][color] >= turn;
                    const bool is_improving =
                        nb_conflicts == 0 and solution.nb_uncolored() <= best_found;
                    if (is_move_tabu and not is_improving) {
                        continue;
                    }

                    if (nb_conflicts < best_current) {
                        best_current = nb_conflicts;
                        best_colorations.clear();
                    }
                    best_colorations.emplace_back(Coloration{vertex, color});
                }
            }
            // If no move, pick a random one
            if (best_colorations.empty()) {
                const int vertex = rd::choice(solution.uncolored());
                const int color = distribution_colors(rd::generator);
                best_colorations.emplace_back(Coloration{vertex, color});
            }

            const auto [vertex, color] = rd::choice(best_colorations);
            solution.grenade_move_optimized_2(vertex, color);

            // Block neighbor of the best move from coming to the color
            for (const int neighbor : graph->neighborhood[vertex]) {
                long t_tenure =
                    static_cast<long>(param.alpha *
                                      static_cast<double>(solution.nb_uncolored())) +
                    distribution_tabu(rd::generator);
                tabu_matrix[neighbor][color] = turn + t_tenure;
            }

            assert(solution.check_solution());

            if (solution.nb_uncolored() < best_found) {
                best_found = solution.nb_uncolored();
                best_solution = solution;
                best_time =
                    parameters->elapsed_time(std::chrono::high_resolution_clock::now());
                best_turn = turn;
                if (param.verbose) {
                    print_result_ls(best_time, best_solution, turn);
                }
            }
        }
        if (solution.is_legal()) {
            best_legal_solution = solution;
            best_legal_time = best_time;
            best_legal_turn = best_turn;
        }
    }
    if (param.verbose) {
        print_result_ls(best_time, best_solution, best_turn);
        if (best_legal_solution) {
            print_result_ls(best_legal_time, best_legal_solution, best_legal_turn);
        }
    }
    if (best_solution.nb_uncolored() != 0) {
        best_solution.color_uncolored();
    }
    assert(best_solution.check_solution());
    return best_legal_solution;
}

bool M_1_2_3(Solution &solution, const long turn, std::vector<long> &tabu_list) {
    // M1 : moves a vertex to a free color

    // M2 : moves a vertex and moves its neighbors to other colors (perfect grenade)

    // M3 : moves a vertex without increasing the score and moves its
    // neighbors to other colors except for one that become uncolored (grenade one lost)

    // list(vertex, color) for M3 grenade
    std::vector<std::tuple<int, int>> grenade_one_lost;
    for (const auto &vertex : solution.uncolored()) {

        // M1 : move vertex to a color with no neighbors
        const auto &possible_colors = solution.possible_colors(vertex);
        if (not possible_colors.empty()) {
            solution.grenade_move_optimized(vertex, rd::choice(possible_colors));
            // fmt::print("M1 : vertex {} to color {}\n", vertex, solution[vertex]);
            return true;
        }

        // M2 : move vertex to a color with neighbors (prefect grenade)

        // costs counts the number of neighbors in the tabu list
        std::vector<int> costs(solution.nb_colors(), 0);
        // relocated counts the number of neighbors that must be relocated for each color
        std::vector<int> relocated(solution.nb_colors(), 0);
        for (const auto &neighbor : graph->neighborhood[vertex]) {
            // if neighbor is unassigned, we don't care
            int neighbor_color = solution[neighbor];
            if (neighbor_color == -1) {
                continue;
            }

            // if the number of free colors for the neighbor is > 0, we increase the
            // number of relocated for the color
            if (not solution.possible_colors(neighbor).empty()) {
                ++relocated[neighbor_color];
            } else if (tabu_list[neighbor] < turn) {
                ++relocated[neighbor_color];
                ++costs[neighbor_color];
            }

            // if not all neighbors in the color have been explored, we continue
            if (relocated[neighbor_color] !=
                solution.nb_conflicts(vertex, neighbor_color)) {
                continue;
            }

            // if all neighbors can be relocated, we apply the move
            if (costs[neighbor_color] == 0) {
                // we apply the grenade move on the vertex and neighbors
                solution.grenade_move_optimized(vertex, neighbor_color);
                // fmt::print("M2 : vertex {} to color {}\n", vertex, neighbor_color);
                return true;
            }
            // if there is only one neighbor that become uncolored, keep the move for M3
            if (costs[neighbor_color] == 1) {
                grenade_one_lost.emplace_back(vertex, neighbor_color);
            }
        }
    }

    // M3 : move vertex to a color with neighbors, only one neighbor become uncolored
    if (grenade_one_lost.empty()) {
        return false;
    }
    const auto [vertex, min_color] = rd::choice(grenade_one_lost);
    solution.grenade_move_optimized(vertex, min_color);
    tabu_list[vertex] = turn + static_cast<long>(solution.nb_colors());
    // fmt::print("M3 : vertex {} to color {}\n", vertex, min_color);
    return true;
}

bool M_4(Solution &solution,
         const long turn,
         std::vector<long> &tabu_list,
         const std::vector<int> &vertices) {
    // M4 : for each colored vertex not tabu with free colors, move it to an other color
    // move at most |non_empty_colors| vertices
    int counter = 0;
    for (const auto &vertex : vertices) {
        if (not solution.possible_colors(vertex).empty() and tabu_list[vertex] < turn and
            solution[vertex] != -1) {
            tabu_list[vertex] = turn + static_cast<long>(solution.nb_colors());
            solution.grenade_move_optimized(vertex,
                                            rd::choice(solution.possible_colors(vertex)));
            // fmt::print("M4 : vertex {} to color {}\n", vertex, solution[vertex]);
            ++counter;
            if (counter == solution.nb_colors()) {
                return true;
            }
        }
    }
    return (counter > 0);
}

bool M_5(Solution &solution,
         const long turn,
         std::vector<long> &tabu_list,
         const std::vector<int> &vertices) {
    // M5 : for each vertex try to relocate its neighbors

    for (const auto &vertex : vertices) {
        const bool has_free_colors = not solution.possible_colors(vertex).empty();
        const bool is_not_tabu = tabu_list[vertex] >= turn;
        const bool is_not_colored = solution[vertex] == -1;

        if (has_free_colors or is_not_tabu or is_not_colored) {
            continue;
        }
        std::vector<int> relocated(solution.nb_colors(), 0);
        for (const auto &neighbor : graph->neighborhood[vertex]) {
            int c_neighbor = solution[neighbor];
            if (c_neighbor == -1)
                continue;
            if (not solution.possible_colors(neighbor).empty()) {
                ++relocated[c_neighbor];
            }

            if (relocated[c_neighbor] == solution.nb_conflicts(vertex, c_neighbor)) {
                solution.grenade_move_optimized(vertex, c_neighbor);
                tabu_list[vertex] = turn + static_cast<long>(solution.nb_colors());
                // fmt::print("M5 : vertex {} to color {}\n", vertex, c_neighbor);
                return true;
            }
        }
    }
    return false;
}

bool M_6(Solution &solution, const long turn, std::vector<long> &tabu_list) {
    // M6 : pick a random uncolored vertex and try to relocate its neighbors
    const int vertex = rd::choice(solution.uncolored());

    std::vector<int> relocated(solution.nb_colors(), 0);
    std::vector<int> costs(solution.nb_colors(), 0);

    std::vector<int> best_grenade;
    int min_cost = graph->nb_vertices;

    for (const auto &neighbor : graph->neighborhood[vertex]) {
        const int c_neighbor = solution[neighbor];
        if (c_neighbor == -1)
            continue;
        if (not solution.possible_colors(neighbor).empty()) {
            ++relocated[c_neighbor];
        } else {
            ++relocated[c_neighbor];
            ++costs[c_neighbor];
        }
        if (relocated[c_neighbor] != solution.nb_conflicts(vertex, c_neighbor)) {
            continue;
        }
        if (costs[c_neighbor] > min_cost) {
            continue;
        }

        if (costs[c_neighbor] < min_cost) {
            min_cost = costs[c_neighbor];
            best_grenade.clear();
        }

        best_grenade.emplace_back(c_neighbor);
    }

    if (best_grenade.empty()) {
        return false;
    }

    std::fill(tabu_list.begin(), tabu_list.end(), 0);
    solution.grenade_move_optimized(vertex, rd::choice(best_grenade));
    tabu_list[vertex] = turn + static_cast<long>(solution.nb_colors());
    // fmt::print("M6 : vertex {} to color {}\n", vertex, solution[vertex]);
    return true;
}

std::optional<Solution> partial_ts(Solution &best_solution, const ParamLS &param) {
    const auto max_time =
        std::chrono::high_resolution_clock::now() + std::chrono::seconds(param.max_time);

    int64_t best_time = 0;
    int64_t best_turn = 0;
    int64_t best_legal_time = 0;
    int64_t best_legal_turn = 0;
    std::optional<Solution> best_legal_solution = std::nullopt;
    if (best_solution.is_legal()) {
        best_legal_solution = best_solution;
    }

    if (best_solution.penalty() != 0) {
        best_solution.remove_penalty();
    }

    if (parameters->use_target and best_solution.nb_colors() > parameters->nb_colors) {
        // remove all excess colors
        best_solution =
            best_solution.reduce_nb_colors_partial_legal(parameters->nb_colors);
        assert(best_solution.check_solution());
    }

    Solution solution = best_solution;

    long turn = 0;

    std::vector<int> vertices;
    vertices.resize(graph->nb_vertices);
    std::iota(vertices.begin(), vertices.end(), 0);

    while (not parameters->time_limit_reached_sub_method(max_time) and
           turn < param.max_iterations and
           not(best_solution.is_legal() and
               best_solution.nb_colors() == parameters->nb_colors)) {

        if (solution.is_legal() and not parameters->use_target) {
            solution = solution.reduce_nb_colors_partial_legal(solution.nb_colors() - 1);
            best_solution = solution;
            if (param.verbose) {
                print_result_ls(best_time, best_solution, turn);
            }
            assert(solution.check_solution());
        }

        solution.init_possible_colors();

        int best_found = solution.nb_uncolored();
        std::vector<long> tabu_list(graph->nb_vertices, 0);

        std::uniform_int_distribution<int> distribution_colors(0,
                                                               solution.nb_colors() - 1);

        // vector of colors from 0 to nb_colors -1
        std::vector<int> colors(solution.nb_colors());
        std::iota(colors.begin(), colors.end(), 0);

        turn = 0;
        while (not parameters->time_limit_reached_sub_method(max_time) and
               turn < param.max_iterations and not best_solution.is_legal()) {

            ++turn;
            bool change = false;
            std::shuffle(vertices.begin(), vertices.end(), rd::generator);
            std::shuffle(colors.begin(), colors.end(), rd::generator);

            if (M_1_2_3(solution, turn, tabu_list)) {
                change = true;
                assert(solution.check_solution());
            } else if (M_4(solution, turn, tabu_list, vertices)) {
                change = true;
                assert(solution.check_solution());
            } else if (M_5(solution, turn, tabu_list, vertices)) {
                change = true;
                assert(solution.check_solution());
            } else if (M_6(solution, turn, tabu_list)) {
                change = true;
                assert(solution.check_solution());
            }
            if (not change) {
                continue;
            }
            assert(solution.check_solution());

            if (solution.nb_uncolored() < best_found) {
                best_found = solution.nb_uncolored();
                best_solution = solution;
                best_time =
                    parameters->elapsed_time(std::chrono::high_resolution_clock::now());
                best_turn = turn;
                if (param.verbose) {
                    print_result_ls(best_time, best_solution, turn);
                }
            }
        }
        if (solution.is_legal()) {
            best_legal_solution = solution;
            best_legal_time = best_time;
            best_legal_turn = best_turn;
        }
    }
    if (param.verbose) {
        print_result_ls(best_time, best_solution, best_turn);
        if (best_legal_solution) {
            print_result_ls(best_legal_time, best_legal_solution, best_legal_turn);
        }
    }
    if (best_solution.nb_uncolored() != 0) {
        best_solution.color_uncolored();
    }
    assert(best_solution.check_solution());
    return best_legal_solution;
}

std::optional<Solution> tabu_col(Solution &best_solution, const ParamLS &param) {
    const auto max_time =
        std::chrono::high_resolution_clock::now() + std::chrono::seconds(param.max_time);

    int64_t best_time = 0;
    int64_t best_turn = 0;
    int64_t best_legal_time = 0;
    int64_t best_legal_turn = 0;
    std::optional<Solution> best_legal_solution = std::nullopt;
    if (best_solution.is_legal()) {
        best_legal_solution = best_solution;
    }

    std::uniform_int_distribution<long> distribution_tabu(param.random_min,
                                                          param.random_max);

    if (best_solution.nb_uncolored() != 0) {
        best_solution.color_uncolored();
    }

    if (parameters->use_target and best_solution.nb_colors() > parameters->nb_colors) {
        // remove all excess colors
        best_solution = best_solution.reduce_nb_colors_illegal(parameters->nb_colors);
        assert(best_solution.check_solution());
    }

    Solution solution = best_solution;

    long turn = 0;

    while (not parameters->time_limit_reached_sub_method(max_time) and
           turn < param.max_iterations and
           not(best_solution.is_legal() and
               best_solution.nb_colors() == parameters->nb_colors)) {

        if (solution.is_legal() and not parameters->use_target) {
            // remove one color
            solution = solution.reduce_nb_colors_illegal(solution.nb_colors() - 1);

            best_solution = solution;
            if (param.verbose) {
                print_result_ls(best_time, best_solution, turn);
            }
            assert(solution.check_solution());
        }

        solution.init_deltas();

        int best_found = solution.penalty();
        std::vector<std::vector<long>> tabu_matrix(
            graph->nb_vertices, std::vector<long>(solution.nb_colors(), 0));

        turn = 0;
        while (not parameters->time_limit_reached_sub_method(max_time) and
               turn < param.max_iterations and not best_solution.is_legal()) {

            ++turn;

            int best_current = std::numeric_limits<int>::max();
            std::vector<Coloration> best_colorations;

            for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
                if (solution.nb_conflicts(vertex) == 0) {
                    continue;
                }
                for (int color = 0; color < solution.nb_colors(); ++color) {
                    if (color == solution[vertex]) {
                        continue;
                    }
                    const int delta_conflict =
                        solution.delta_conflicts_colors(vertex, color);
                    if (delta_conflict > best_current) {
                        continue;
                    }
                    const bool is_move_tabu = tabu_matrix[vertex][color] >= turn;
                    const bool is_improving =
                        solution.penalty() + delta_conflict < best_found;
                    if (is_move_tabu and not is_improving) {
                        continue;
                    }
                    if (delta_conflict < best_current) {
                        best_current = delta_conflict;
                        best_colorations.clear();
                    }
                    best_colorations.emplace_back(Coloration{vertex, color});
                }
            }
            if (best_colorations.empty()) {
                continue;
            }

            const auto [vertex, color] = rd::choice(best_colorations);
            const int old_color = solution.move_to_color(vertex, color);

            tabu_matrix[vertex][old_color] =
                turn + distribution_tabu(rd::generator) +
                static_cast<long>(
                    static_cast<double>(solution.conflicting_vertices().size()) *
                    param.alpha);

            assert(solution.check_solution());

            if (solution.penalty() < best_found) {
                best_found = solution.penalty();
                best_solution = solution;
                best_time =
                    parameters->elapsed_time(std::chrono::high_resolution_clock::now());
                best_turn = turn;
                if (param.verbose) {
                    print_result_ls(best_time, best_solution, turn);
                }
            }
        }
        if (solution.is_legal()) {
            best_legal_solution = solution;
            best_legal_time = best_time;
            best_legal_turn = best_turn;
        }
    }
    if (param.verbose) {
        print_result_ls(best_time, best_solution, best_turn);
        if (best_legal_solution) {
            print_result_ls(best_legal_time, best_legal_solution, best_legal_turn);
        }
    }
    assert(best_solution.check_solution());
    return best_legal_solution;
}

std::optional<Solution> tabu_col_optimized(Solution &best_solution,
                                           const ParamLS &param) {
    const auto max_time =
        std::chrono::high_resolution_clock::now() + std::chrono::seconds(param.max_time);

    int64_t best_time = 0;
    int64_t best_turn = 0;
    int64_t best_legal_time = 0;
    int64_t best_legal_turn = 0;
    std::optional<Solution> best_legal_solution = std::nullopt;
    if (best_solution.is_legal()) {
        best_legal_solution = best_solution;
    }

    std::uniform_int_distribution<long> distribution_tabu(param.random_min,
                                                          param.random_max);
    if (best_solution.nb_uncolored() != 0) {
        best_solution.color_uncolored();
    }

    if (parameters->use_target and best_solution.nb_colors() > parameters->nb_colors) {
        // remove all excess colors
        best_solution = best_solution.reduce_nb_colors_illegal(parameters->nb_colors);
    }

    Solution solution = best_solution;

    long turn = 0;

    while (not parameters->time_limit_reached_sub_method(max_time) and
           turn < param.max_iterations and
           not(best_solution.is_legal() and
               best_solution.nb_colors() == parameters->nb_colors)) {

        if (solution.is_legal() and not parameters->use_target) {
            // remove one color
            solution = solution.reduce_nb_colors_illegal(solution.nb_colors() - 1);
            best_solution = solution;
            if (param.verbose) {
                print_result_ls(best_time, best_solution, turn);
            }
            assert(solution.check_solution());
        }
        solution.init_deltas_optimized();

        int best_found = solution.penalty();
        std::vector<std::vector<long>> tabu_matrix(
            graph->nb_vertices, std::vector<long>(solution.nb_colors(), 0));

        turn = 0;

        while (not parameters->time_limit_reached_sub_method(max_time) and
               turn < param.max_iterations and not best_solution.is_legal()) {

            ++turn;

            int best_nb_conflicts = std::numeric_limits<int>::max();
            std::vector<Coloration> best_colorations;

            for (const auto vertex : solution.conflicting_vertices()) {
                const int current_best_improve = solution.best_delta(vertex);
                if (current_best_improve > best_nb_conflicts) {
                    continue;
                }

                const int current_color = solution[vertex];
                bool added = false;

                for (const auto &color : solution.best_improve_colors(vertex)) {
                    if (color == current_color) {
                        continue;
                    }

                    const bool vertex_tabu = (tabu_matrix[vertex][color] >= turn);
                    const bool improve_best_solution =
                        (((current_best_improve + solution.penalty()) < best_found));

                    if (not improve_best_solution and vertex_tabu) {
                        continue;
                    }

                    added = true;
                    if (current_best_improve < best_nb_conflicts) {
                        best_nb_conflicts = current_best_improve;
                        best_colorations.clear();
                    }
                    best_colorations.emplace_back(Coloration{vertex, color});
                }

                if (added or (current_best_improve >= best_nb_conflicts)) {
                    continue;
                }

                // if the bests moves are tabu we have to look for other moves
                for (int color = 0; color < solution.nb_colors(); color++) {
                    if (color == current_color) {
                        continue;
                    }

                    const int conflicts = solution.delta_conflicts_colors(vertex, color);
                    if (conflicts > best_nb_conflicts) {
                        continue;
                    }

                    const bool vertex_tabu = (tabu_matrix[vertex][color] >= turn);
                    const bool improve_best_solution =
                        (((current_best_improve + solution.penalty()) < best_found));
                    if (not improve_best_solution and vertex_tabu) {
                        continue;
                    }

                    if ((conflicts < best_nb_conflicts)) {
                        best_nb_conflicts = conflicts;
                        best_colorations.clear();
                    }
                    best_colorations.emplace_back(Coloration{vertex, color});
                }
            }
            if (best_colorations.empty()) {
                continue;
            }

            // select and apply the best move
            const auto [vertex, color] = rd::choice(best_colorations);
            const int old_color = solution.move_to_color_optimized(vertex, color);

            // update tabu matrix
            tabu_matrix[vertex][old_color] =
                turn + distribution_tabu(rd::generator) +
                static_cast<long>(
                    param.alpha *
                    static_cast<double>(solution.conflicting_vertices().size()));

            assert(solution.check_solution());

            if (solution.penalty() < best_found) {
                best_found = solution.penalty();
                best_solution = solution;
                best_time =
                    parameters->elapsed_time(std::chrono::high_resolution_clock::now());
                best_turn = turn;
                if (param.verbose) {
                    print_result_ls(best_time, best_solution, turn);
                }
            }
        }
        if (solution.is_legal()) {
            best_legal_solution = solution;
            best_legal_time = best_time;
            best_legal_turn = best_turn;
        }
    }
    if (param.verbose) {
        print_result_ls(best_time, best_solution, best_turn);
        if (best_legal_solution) {
            print_result_ls(best_legal_time, best_legal_solution, best_legal_turn);
        }
    }
    assert(best_solution.check_solution());
    return best_legal_solution;
}

bool check_solution(const std::vector<bool> &_solution,
                    const int _score_UBQP,
                    const int _penalty,
                    const std::vector<int> &_delta_scores,
                    std::map<int, std::vector<int>> _buckets) {
    (void)_score_UBQP;   // only used in assert
    (void)_penalty;      // only used in assert
    (void)_delta_scores; // only used in assert
    (void)_buckets;      // only used in assert

    int score_UBQP = 0;
    // compute score
    for (int arc1 = 0; arc1 < qgraph->nb_arc; ++arc1) {
        for (int arc2 = 0; arc2 <= arc1; ++arc2) {
            score_UBQP += qgraph->qmatrix[arc1][arc2] * _solution[arc1] * _solution[arc2];
        }
    }
    assert(score_UBQP == _score_UBQP);

    std::vector<int> delta_scores(qgraph->nb_arc, 0);
    int penalty = 0;

    // init delta score
    for (int arc = 0; arc < qgraph->nb_arc; ++arc) {
        delta_scores[arc] = qgraph->qmatrix[arc][arc];
        for (const auto neighbor : qgraph->neighborhood[arc]) {
            delta_scores[arc] += qgraph->qmatrix[arc][neighbor] * _solution[neighbor];
            if (_solution[arc] and _solution[neighbor]) {
                ++penalty;
            }
        }
        if (_solution[arc] == 1) {
            delta_scores[arc] = -delta_scores[arc];
        }
    }
    assert(delta_scores == _delta_scores);

    penalty /= 2;
    assert(penalty == _penalty);

    std::map<int, std::vector<int>> buckets;

    // init buckets
    for (int arc = 0; arc < qgraph->nb_arc; ++arc) {
        insert_sorted(buckets[delta_scores[arc]], arc);
    }

    for (const auto &[delta, arcs] : buckets) {
        (void)delta;
        (void)arcs;
        assert(arcs == _buckets.at(delta));
    }

    return true;
}

std::optional<Solution> tabu_bucket(Solution &best_solution, const ParamLS &param) {
    const auto max_time =
        std::chrono::high_resolution_clock::now() + std::chrono::seconds(param.max_time);

    int64_t best_time = 0;

    std::uniform_int_distribution<long> distribution_tabu(param.random_min,
                                                          param.random_max);

    // in case the original graph is a clique
    if (qgraph->nb_arc == 0) {
        if (param.verbose) {
            print_result_ls(best_time, best_solution, 0);
        }
        return std::nullopt;
    }

    std::vector<bool> solution(qgraph->nb_arc, false);

    // convert solution
    // activate arcs
    std::vector<std::vector<int>> color_groups(best_solution.nb_colors());
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        color_groups[best_solution[vertex]].emplace_back(vertex);
    }
    for (int arc = 0; arc < qgraph->nb_arc; ++arc) {
        const auto &[tail, head] = qgraph->arcs[arc];
        const int color_tail = best_solution[tail];
        const int color_head = best_solution[head];
        if (color_tail == color_head and color_groups[color_tail][0] == head) {
            solution[arc] = true;
        }
    }

    std::vector<bool> best_ubqp_solution = solution;

    // for (int arc1 = 0; arc1 < qgraph->nb_arc; ++arc1) {
    //     fmt::print("{:02d} : ", arc1);
    //     for (int arc2 = 0; arc2 <= arc1; ++arc2) {
    //         if (_solution[arc1] and _solution[arc2]) {
    //             fmt::print(
    //                 "{} ",
    //                 fmt::styled(qgraph->qmatrix[arc1][arc2],
    //                 fmt::bg(fmt::color::red)));
    //         } else {
    //             fmt::print("{} ", qgraph->qmatrix[arc1][arc2]);
    //         }
    //     }
    //     fmt::print("\n");
    // }

    // compute score
    int score = 0;
    for (int arc1 = 0; arc1 < qgraph->nb_arc; ++arc1) {
        for (int arc2 = 0; arc2 <= arc1; ++arc2) {
            score += qgraph->qmatrix[arc1][arc2] * solution[arc1] * solution[arc2];
        }
    }
    int best_score = score;

    // init delta score and penalty
    int penalty = 0;
    std::vector<int> delta_scores(qgraph->nb_arc, 0);
    for (int arc = 0; arc < qgraph->nb_arc; ++arc) {
        delta_scores[arc] = qgraph->qmatrix[arc][arc];
        const bool activated = solution[arc];
        for (const auto neighbor : qgraph->neighborhood[arc]) {
            delta_scores[arc] += qgraph->qmatrix[arc][neighbor] * solution[neighbor];
            if (activated and solution[neighbor]) {
                ++penalty;
            }
        }
        if (solution[arc]) {
            delta_scores[arc] = -delta_scores[arc];
        }
    }
    penalty /= 2;

    // insert delta of swap of arc into the buckets
    std::map<int, std::vector<int>> buckets;
    // std::map<int, std::set<int>> buckets;
    for (int arc = 0; arc < qgraph->nb_arc; ++arc) {
        insert_sorted(buckets[delta_scores[arc]], arc);
        // buckets[delta_scores[arc]].insert(arc);
    }

    if (param.verbose) {
        fmt::print(parameters->output,
                   "-1,{},0,{},{},{}\n",
                   best_time,
                   penalty,
                   graph->nb_vertices + score,
                   score);
    }

    std::vector<long> tabu_list(qgraph->nb_arc, 0);
    long min_tabu = static_cast<long>(qgraph->nb_arc * param.alpha);
    long turn = 0;

    while (not parameters->time_limit_reached_sub_method(max_time) and
           turn < param.max_iterations and
           (not parameters->use_target or
            (penalty != 0 or (graph->nb_vertices + score != parameters->nb_colors)))) {
        ++turn;
        // std::vector<int> best_arcs;
        int best_arc = -1;
        for (const auto &[delta, bucket] : buckets) {
            if (bucket.empty()) {
                continue;
            }
            const int bucket_size = static_cast<int>(bucket.size());
            const bool aspiration_criteria = delta + score < best_score;

            std::uniform_int_distribution<int> distribution_bucket(0, bucket_size - 1);
            auto random_position = distribution_bucket(rd::generator);
            auto it_bucket = bucket.begin();
            std::advance(it_bucket, random_position);
            for (int i = random_position; i < bucket_size; ++i) {
                if (tabu_list[*it_bucket] <= turn or aspiration_criteria) {
                    best_arc = *it_bucket;
                    break;
                }
                ++it_bucket;
            }
            if (best_arc != -1) {
                break;
            }
            it_bucket = bucket.begin();
            for (int i = 0; i < bucket_size - random_position; ++i) {
                if (tabu_list[*it_bucket] <= turn or aspiration_criteria) {
                    best_arc = *it_bucket;
                    break;
                }
                ++it_bucket;
            }
            if (best_arc != -1) {
                break;
            }

            // for (const auto arc : bucket) {
            //     if (tabu_list[arc] <= turn or aspiration_criteria) {
            //         best_arcs.emplace_back(arc);
            //     }
            // }
            // if (best_arcs.empty()) {
            //     continue;
            // }
            // break;
        }
        // if (best_arcs.empty()) {
        //     continue;
        // }
        // int arc = rd::choice(best_arcs);
        if (best_arc == -1) {
            continue;
        }

        // switch arc
        const int old_delta = delta_scores[best_arc];
        const int new_delta = -old_delta;
        score += old_delta;
        delta_scores[best_arc] = new_delta;
        erase_sorted(buckets[old_delta], best_arc);
        insert_sorted(buckets[new_delta], best_arc);
        // buckets[old_delta].erase(best_arc);
        // buckets[new_delta].insert(best_arc);

        const bool old_activation = solution[best_arc];
        for (const auto neighbor : qgraph->neighborhood[best_arc]) {
            const int old_delta_n = delta_scores[neighbor];
            if (old_activation == solution[neighbor]) {
                delta_scores[neighbor] += qgraph->qmatrix[best_arc][neighbor];
            } else {
                delta_scores[neighbor] -= qgraph->qmatrix[best_arc][neighbor];
            }
            if (solution[neighbor]) {
                if (old_activation) {
                    --penalty;
                } else {
                    ++penalty;
                }
            }
            const int new_delta_n = delta_scores[neighbor];
            erase_sorted(buckets[old_delta_n], neighbor);
            insert_sorted(buckets[new_delta_n], neighbor);
            // buckets[old_delta_n].erase(neighbor);
            // buckets[new_delta_n].insert(neighbor);
        }
        solution[best_arc] = not solution[best_arc];

        tabu_list[best_arc] = turn + distribution_tabu(rd::generator) + min_tabu;

        // assert(solution.check_solution());
        assert(check_solution(solution, score, penalty, delta_scores, buckets));

        if (score < best_score) {
            best_score = score;
            best_ubqp_solution = solution;
            best_time =
                parameters->elapsed_time(std::chrono::high_resolution_clock::now());
            if (param.verbose) {
                fmt::print(parameters->output,
                           "{},{},0,{},{},{}\n",
                           turn,
                           best_time,
                           penalty,
                           graph->nb_vertices + best_score,
                           score);
            }
        }
    }
    best_solution = best_ubqp_solution;

    if (param.verbose) {
        print_result_ls(best_time, best_solution, turn);
    }
    return std::nullopt;
}
