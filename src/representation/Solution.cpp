#include "Solution.hpp"

#include <algorithm>
#include <cassert>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"

#ifdef DEBUG
#include <fmt/color.h>
#endif

#include <fmt/printf.h>
#pragma GCC diagnostic pop

#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"
#include "../utils/utils.hpp"
#include "Graph.hpp"

using namespace graph_instance;
using namespace parameters_search;

int Solution::best_penalty = std::numeric_limits<int>::max();
int Solution::best_nb_colors = std::numeric_limits<int>::max();

ulong Solution::counter = 0;
const std::string Solution::header_csv = "nb_uncolored,penalty,nb_colors,solution";

Solution::Solution()
    : id(counter++),
      _colors(graph->nb_vertices, -1),
      _nb_colors(0),
      _uncolored(graph->nb_vertices),
      _penalty(0) {
    std::iota(_uncolored.begin(), _uncolored.end(), 0);
}

Solution::Solution(const std::vector<std::vector<int>> &solution)
    : _colors(graph->nb_vertices, -1), _nb_colors(0), _penalty(0) {
    for (const auto &group : solution) {
        int color = -1;
        for (const auto vertex : group) {
            color = add_to_color(vertex, color);
        }
    }
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        if (_colors[vertex] != -1) {
            continue;
        }
        std::vector<int> possible_colors;
        int min_conflicts = std::numeric_limits<int>::max();
        for (int color = 0; color < _nb_colors; ++color) {
            int nb_conflicts = _conflicts[color][vertex];
            if (nb_conflicts > min_conflicts) {
                continue;
            }
            if (nb_conflicts < min_conflicts) {
                min_conflicts = nb_conflicts;
                possible_colors.clear();
            }
            possible_colors.emplace_back(color);
        }
        add_to_color(vertex, rd::choice(possible_colors));
    }
}

Solution::Solution(const std::vector<bool> &solution)
    : _colors(graph->nb_vertices, -1),
      _nb_colors(0),
      _uncolored(graph->nb_vertices),
      _penalty(0) {
    std::iota(_uncolored.begin(), _uncolored.end(), 0);
    std::vector<int> dominants(graph->nb_vertices, -1);
    std::vector<std::vector<int>> color_groups;
    // color_groups.reserve(nb_max_colors);

    int nb_colors = 0;
    // find the connected vertices
    // for each activated arc
    for (int arc = 0; arc < qgraph->nb_arc; ++arc) {
        if (not solution[arc]) {
            continue;
        }
        bool pass = false;
        for (const auto neighbor : qgraph->neighborhood[arc]) {
            if (neighbor > arc and solution[neighbor]) {
                pass = true;
                break;
            }
        }
        if (pass) {
            continue;
        }
        const auto &[tail, head] = qgraph->arcs[arc];
        if (dominants[tail] == -1 and dominants[head] == -1) {
            // if none of them as a color
            // the tail become the dominant of a new color
            dominants[tail] = tail;
            dominants[head] = tail;
            color_groups.emplace_back();
            color_groups[nb_colors].emplace_back(tail);
            color_groups[nb_colors].emplace_back(head);
            ++nb_colors;
        } else if (dominants[tail] == -1) {
            // if the head already in a color
            // the tail join the color
            int color_ = -1;
            for (int color = 0; color < nb_colors; ++color) {
                if (color_groups[color][0] == dominants[head]) {
                    color_ = color;
                }
            }
            assert(color_ != -1);
            dominants[tail] = dominants[head];
            color_groups[color_].emplace_back(tail);
        } else if (dominants[head] == -1) {
            // if the tail already in a color
            // the head join the color
            int color_ = -1;
            for (int color = 0; color < nb_colors; ++color) {
                if (color_groups[color][0] == dominants[tail]) {
                    color_ = color;
                }
            }
            assert(color_ != -1);
            dominants[head] = dominants[tail];
            color_groups[color_].emplace_back(head);
        }
    }

    // sort groups by size to ignore afterwards the smallest ones
    std::stable_sort(
        color_groups.begin(), color_groups.end(), [](const auto &cg1, const auto &cg2) {
            return cg1.size() > cg2.size();
        });

    // create the solution with the largest colors
    for (const auto &group : color_groups) {
        int color = -1;
        for (const auto vertex : group) {
            color = add_to_color(vertex, color);
        }
    }
    // while ((parameters->use_target and _nb_colors < parameters->nb_colors) or
    //        (not parameters->use_target and not _uncolored.empty())) {
    while (not _uncolored.empty()) {
        const int vertex = _uncolored[0];
        std::vector<int> possible_colors;
        for (int color = 0; color < _nb_colors; ++color) {
            if (_conflicts[color][vertex] == 0) {
                possible_colors.emplace_back(color);
            }
        }
        if (possible_colors.empty()) {
            possible_colors.emplace_back(-1);
        }

        add_to_color(vertex, rd::choice(possible_colors));
    }
    // while (parameters->use_target and _nb_colors > parameters->nb_colors) {}
}

Solution Solution::reduce_nb_colors_partial_legal(const int nb_colors) const {
    Solution solution;
    std::vector<int> sorted_colors(_color_size.size(), 0);
    std::iota(sorted_colors.begin(), sorted_colors.end(), 0);
    const auto &color_size = _color_size;
    std::stable_sort(
        sorted_colors.begin(), sorted_colors.end(), [&color_size](int c1, int c2) {
            return color_size[c1] > color_size[c2];
        });
    for (int i = 0; i < nb_colors; ++i) {
        const int next_group = sorted_colors[i];
        int color = -1;
        for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
            if (_colors[vertex] == next_group) {
                color = solution.add_to_color(vertex, color);
            }
        }
    }
    assert(solution.check_solution());

    int i = 0;
    while (i < solution.nb_uncolored()) {
        const int vertex = solution._uncolored[i];
        bool colored = false;
        for (int color = 0; color < solution.nb_colors(); ++color) {
            if (solution.nb_conflicts(vertex, color) == 0) {
                solution.add_to_color(vertex, color);
                colored = true;
                break;
            }
        }
        if (not colored) {
            ++i;
        }
    }
    assert(check_solution());
    return solution;
}

Solution Solution::reduce_nb_colors_illegal(const int nb_colors) const {
    // create groups of colors containing the vertices in each color
    std::vector<std::vector<int>> color_groups;
    // fill the color_groups with the vertices in each color
    for (int color = 0; color < _nb_colors; ++color) {
        color_groups.emplace_back();
        for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
            if (_colors[vertex] == color) {
                color_groups[color].emplace_back(vertex);
            }
        }
    }
    // sort the groups by size, decreasing
    std::stable_sort(
        color_groups.begin(), color_groups.end(), [](const auto &cg1, const auto &cg2) {
            return cg1.size() > cg2.size();
        });

    // keep only the nb_colors largest groups
    color_groups.resize(nb_colors);

    Solution solution(color_groups);
    solution.id = id;

    return solution;
}

void Solution::init_deltas() {
    _deltas = std::vector<std::vector<int>>(_nb_colors,
                                            std::vector<int>(graph->nb_vertices, 0));

    for (int vertex = 0; vertex < graph->nb_vertices; vertex++) {
        int current = 0;
        if (_colors[vertex] == -1) {
            current = 0;
        } else {
            current = _conflicts[_colors[vertex]][vertex];
        }
        for (int color = 0; color < _nb_colors; ++color) {

            _deltas[color][vertex] = _conflicts[color][vertex] - current;
        }
    }
}

void Solution::init_deltas_optimized() {
    init_deltas();

    _best_delta = std::vector<int>(graph->nb_vertices, graph->nb_vertices);
    _best_improve_colors = std::vector<std::vector<int>>(graph->nb_vertices);
    for (int vertex = 0; vertex < graph->nb_vertices; vertex++) {
        for (int color = 0; color < _nb_colors; ++color) {
            const int delta = _deltas[color][vertex];
            if (delta < _best_delta[vertex]) {
                _best_delta[vertex] = delta;
                _best_improve_colors[vertex].clear();
            }
            if (delta == _best_delta[vertex]) {
                _best_improve_colors[vertex].emplace_back(color);
            }
        }
    }
}

void Solution::init_possible_colors() {
    _possible_colors = std::vector<std::vector<int>>(graph->nb_vertices);
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        for (int color = 0; color < _nb_colors; ++color) {
            if (_conflicts[color][vertex] == 0 and color != _colors[vertex]) {
                _possible_colors[vertex].emplace_back(color);
            }
        }
    }
}

void Solution::remove_penalty() {
    while (not _conflicting_vertices.empty()) {
        const int vertex = _conflicting_vertices[0];
        delete_from_color(vertex);
    }
    assert(check_solution());
}

void Solution::color_uncolored() {
    _possible_colors.clear();
    _deltas.clear();
    _best_improve_colors.clear();
    _best_delta.clear();
    while (not _uncolored.empty()) {
        const int vertex = _uncolored[0];
        const auto possible_colors = best_possible_colors(vertex);
        const int color = rd::choice(possible_colors);
        // if not all available colors are used, use one of them
        if (parameters->use_target and _nb_colors < parameters->nb_colors and
            _conflicts[color][vertex] != 0) {
            add_to_color(vertex, -1);
        } else {
            add_to_color(vertex, color);
        }
    }
}
void Solution::delete_from_color(const int vertex) {

    const int old_color = _colors[vertex];
    _colors[vertex] = -1;
    assert(old_color != -1);

    // Update conflict score
    const int nb_conflicts_vertex = _conflicts[old_color][vertex];

    _penalty -= nb_conflicts_vertex;
    insert_sorted(_uncolored, vertex);
    if (nb_conflicts_vertex != 0) {
        erase_sorted(_conflicting_vertices, vertex);
    }

    // update conflicts for neighbors
    for (const auto neighbor : graph->neighborhood[vertex]) {
        // for the old color
        --_conflicts[old_color][neighbor];
        if (old_color == _colors[neighbor]) {
            if (_conflicts[old_color][neighbor] == 0) {
                erase_sorted(_conflicting_vertices, neighbor);
            }
        }
    }

    --_color_size[old_color];
    if (_color_size[old_color] == 0) {
        --_nb_colors;
        if (_nb_colors == -1) {
            _nb_colors = 0;
        }
    }
}

int Solution::move_to_color(const int vertex, const int new_color) {
    assert(check_solution());

    const int old_color = _colors[vertex];
    assert(old_color != -1);
    assert(new_color != -1);
    assert(new_color >= 0);
    assert(new_color < _nb_colors);
    assert(_color_size[new_color] != 0);

    _colors[vertex] = new_color;

    // Update conflict score
    const int nb_conflicts_vertex = _conflicts[old_color][vertex];
    const int new_nb_conflicts_vertex = _conflicts[new_color][vertex];
    const int delta = nb_conflicts_vertex - new_nb_conflicts_vertex;

    _penalty -= delta;

    if (nb_conflicts_vertex != 0 and new_nb_conflicts_vertex == 0) {
        erase_sorted(_conflicting_vertices, vertex);
    } else if (nb_conflicts_vertex == 0 and new_nb_conflicts_vertex != 0) {
        insert_sorted(_conflicting_vertices, vertex);
    }

    // update conflicts for neighbors
    for (const auto neighbor : graph->neighborhood[vertex]) {
        // for the old color
        --_deltas[old_color][neighbor];
        --_conflicts[old_color][neighbor];
        if (old_color == _colors[neighbor]) {
            if (_conflicts[old_color][neighbor] == 0) {
                erase_sorted(_conflicting_vertices, neighbor);
            }
            for (int color_ = 0; color_ < _nb_colors; ++color_) {
                ++_deltas[color_][neighbor];
                ++_deltas[color_][vertex];
            }
        }
        // for the new color
        ++_deltas[new_color][neighbor];
        ++_conflicts[new_color][neighbor];
        if (new_color == _colors[neighbor]) {
            if (_conflicts[new_color][neighbor] == 1) {
                insert_sorted(_conflicting_vertices, neighbor);
            }
            // as the presence of the vertex in the color increase the number of
            // conflicts in the color, the delta is better for all other colors
            for (int color_ = 0; color_ < _nb_colors; color_++) {
                --_deltas[color_][neighbor];
                --_deltas[color_][vertex];
            }
        }
    }

    --_color_size[old_color];
    ++_color_size[new_color];

    return old_color;
}

int Solution::move_to_color_optimized(const int vertex, const int new_color) {
    assert(check_solution());

    const int old_color = move_to_color(vertex, new_color);
    for (const auto &neighbor : graph->neighborhood[vertex]) {
        // as the vertex leave//enter the color every delta updates
        if (_colors[neighbor] == old_color) {
            ++_best_delta[neighbor];
            ++_best_delta[vertex];
        }
        if (_colors[neighbor] == new_color) {
            --_best_delta[neighbor];
            --_best_delta[vertex];
        }
        //// ajout pour garder la meilleur transition
        const int best_improve = _best_delta[neighbor];

        if (_deltas[old_color][neighbor] < best_improve) {
            _best_delta[neighbor]--;
            _best_improve_colors[neighbor].clear();
            insert_sorted(_best_improve_colors[neighbor], old_color);
        } else if (_deltas[old_color][neighbor] == best_improve) {
            insert_sorted(_best_improve_colors[neighbor], old_color);
        }

        if ((_deltas[new_color][neighbor] - 1) == best_improve) {

            if (_best_improve_colors[neighbor].size() > 1) {
                assert(contains(_best_improve_colors[neighbor], new_color));
                erase_sorted(_best_improve_colors[neighbor], new_color);
            } else {
                _best_improve_colors[neighbor].clear();
                _best_delta[neighbor] = graph->nb_vertices;
                for (int color_ = 0; color_ < _nb_colors; ++color_) {
                    const int delta = _deltas[color_][neighbor];
                    if (delta < _best_delta[neighbor]) {
                        _best_delta[neighbor] = delta;
                        _best_improve_colors[neighbor].clear();
                    }
                    if (delta == _best_delta[neighbor]) {
                        insert_sorted(_best_improve_colors[neighbor], color_);
                    }
                }
            }
        }
    }
    assert(check_solution());

    return old_color;
}

int Solution::add_to_color(const int vertex, int proposed_color) {
    assert(_colors[vertex] == -1);
    int color = proposed_color;
    if (proposed_color == -1) {
        color = _nb_colors;
        ++_nb_colors;
        _color_size.emplace_back(0);
        _conflicts.emplace_back(graph->nb_vertices, 0);
    }

    _colors[vertex] = color;
    ++_color_size[color];
    const int nb_conflicts = _conflicts[color][vertex];
    if (nb_conflicts != 0) {
        _penalty += nb_conflicts;
        insert_sorted(_conflicting_vertices, vertex);
    }

    erase_sorted(_uncolored, vertex);

    // affect of the vertex entering in the new color
    for (const auto &neighbor : graph->neighborhood[vertex]) {
        ++_conflicts[color][neighbor];
        if (_colors[neighbor] == color) {
            if (_conflicts[color][neighbor] == 1) {
                insert_sorted(_conflicting_vertices, neighbor);
            }
        }
    }
    return color;
}

void Solution::grenade_move(const int vertex, const int color) {
    assert(_colors[vertex] == -1);
    ++_color_size[color];
    _colors[vertex] = color;

    for (const int neighbor : graph->neighborhood[vertex]) {
        ++_conflicts[color][neighbor];
        if (_colors[neighbor] != color) {
            continue;
        }
        --_color_size[color];
        _colors[neighbor] = -1;
        insert_sorted(_uncolored, neighbor);
        for (const int neighbor_2 : graph->neighborhood[neighbor]) {
            --_conflicts[color][neighbor_2];
        }
    }
    erase_sorted(_uncolored, vertex);
}

void Solution::grenade_move_optimized_2(const int vertex, const int color) {
    assert(_colors[vertex] == -1);
    ++_color_size[color];
    _colors[vertex] = color;

    for (const int neighbor : graph->neighborhood[vertex]) {
        ++_conflicts[color][neighbor];
        ++_deltas[color][neighbor];
        if (_colors[neighbor] != color) {
            if (_deltas[color][neighbor] - 1 == _best_delta[neighbor]) {
                if (_best_improve_colors[neighbor].size() > 1) {
                    assert(contains(_best_improve_colors[neighbor], color));
                    erase_sorted(_best_improve_colors[neighbor], color);
                } else {
                    _best_improve_colors[neighbor].clear();
                    _best_delta[neighbor] = graph->nb_vertices;
                    for (int color_ = 0; color_ < _nb_colors; ++color_) {
                        const int delta = _deltas[color_][neighbor];
                        if (delta < _best_delta[neighbor]) {
                            _best_delta[neighbor] = delta;
                            _best_improve_colors[neighbor].clear();
                        }
                        if (delta == _best_delta[neighbor]) {
                            _best_improve_colors[neighbor].emplace_back(color_);
                        }
                    }
                }
            }
        } else {
            --_color_size[color];
            _colors[neighbor] = -1;
            insert_sorted(_uncolored, neighbor);
            if (_best_improve_colors[neighbor].size() > 1) {
                assert(contains(_best_improve_colors[neighbor], color));
                erase_sorted(_best_improve_colors[neighbor], color);
            } else {
                _best_improve_colors[neighbor].clear();
                _best_delta[neighbor] = graph->nb_vertices;
                for (int color_ = 0; color_ < _nb_colors; ++color_) {
                    const int delta = _deltas[color_][neighbor];
                    if (delta < _best_delta[neighbor]) {
                        _best_delta[neighbor] = delta;
                        _best_improve_colors[neighbor].clear();
                    }
                    if (delta == _best_delta[neighbor]) {
                        _best_improve_colors[neighbor].emplace_back(color_);
                    }
                }
            }
            for (const int neighbor_2 : graph->neighborhood[neighbor]) {
                --_conflicts[color][neighbor_2];
                --_deltas[color][neighbor_2];
                if (_deltas[color][neighbor_2] < _best_delta[neighbor_2]) {
                    _best_delta[neighbor_2] = _deltas[color][neighbor_2];
                    _best_improve_colors[neighbor_2].clear();
                    _best_improve_colors[neighbor_2].emplace_back(color);
                } else if (_deltas[color][neighbor_2] == _best_delta[neighbor_2]) {
                    insert_sorted(_best_improve_colors[neighbor_2], color);
                }
            }
        }
    }
    erase_sorted(_uncolored, vertex);
}

void Solution::grenade_move_optimized(const int vertex, const int color) {
    int old_color = _colors[vertex];
    if (old_color != -1) {
        --_color_size[old_color];
        for (const int neighbor : graph->neighborhood[vertex]) {
            --_conflicts[old_color][neighbor];
            if (_conflicts[old_color][neighbor] == 0) {
                insert_sorted(_possible_colors[neighbor], old_color);
            }
        }
    }

    _colors[vertex] = color;
    ++_color_size[color];
    erase_sorted(_uncolored, vertex);

    for (int neighbor : graph->neighborhood[vertex]) {
        ++_conflicts[color][neighbor];
        erase_sorted(_possible_colors[neighbor], color);
        if (_colors[neighbor] == color) {
            --_color_size[color];

            // if the neighbor as no possible colors
            if (_possible_colors[neighbor].empty()) {
                // it is uncolored

                insert_sorted(_uncolored, neighbor);
                _colors[neighbor] = -1;

                // update _conflicts and possible_colors for neighbors
                for (const int neighbor_2 : graph->neighborhood[neighbor]) {
                    --_conflicts[color][neighbor_2];
                    if (_conflicts[color][neighbor_2] == 0 and neighbor_2 != vertex) {
                        insert_sorted(_possible_colors[neighbor_2], color);
                    }
                }
            } else {
                // color the neighbor with a random possible color
                grenade_move_optimized(neighbor, rd::choice(_possible_colors[neighbor]));
            }
        }
        erase_sorted(_possible_colors[neighbor], color);
    }
    erase_sorted(_possible_colors[vertex], color);
    if (old_color != -1 and _conflicts[old_color][vertex] == 0) {
        insert_sorted(_possible_colors[vertex], old_color);
    }
}

bool Solution::check_solution() const {
    int penalty = 0;
    int nb_uncolored = 0;
    for (int vertex = 0; vertex < graph->nb_vertices; vertex++) {
        int current_color = _colors[vertex];
        int nb_conflicts_color = 0;
        if (current_color == -1) {
            assert(contains(_uncolored, vertex));
            ++nb_uncolored;
        } else {
            for (const auto neighbor : graph->neighborhood[vertex]) {
                if (_colors[neighbor] == current_color) {
                    ++nb_conflicts_color;
                }
            }
            assert(nb_conflicts_color == _conflicts[current_color][vertex]);
        }

        int min_delta = graph->nb_vertices;
        std::vector<int> best_colors;
        std::vector<int> possible_colors;
        for (int color = 0; color < _nb_colors; ++color) {
            int nb_conflicts = 0;
            for (const auto neighbor : graph->neighborhood[vertex]) {
                if (_colors[neighbor] == color) {
                    ++nb_conflicts;
                }
            }
            if (nb_conflicts == 0 and color != current_color) {
                possible_colors.push_back(color);
            }
            if (color == current_color) {
                assert(_conflicts[color][vertex] == nb_conflicts);
                if (_penalty == 0) {
                    assert(nb_conflicts == 0);
                }
            }
            assert(nb_conflicts == _conflicts[color][vertex]);
            int delta = nb_conflicts - nb_conflicts_color;
            if (not _deltas.empty()) {
                assert(_deltas[color][vertex] == delta);
            }
            if (color == current_color) {
                penalty += nb_conflicts;
                assert(_conflicts[color][vertex] == nb_conflicts);
                if (nb_conflicts != 0) {
                    assert(contains(_conflicting_vertices, vertex));
                }
            }
            if (delta < min_delta) {
                best_colors.clear();
                min_delta = delta;
            }
            if (delta == min_delta) {
                best_colors.emplace_back(color);
            }
        }
        if (not _possible_colors.empty()) {
            assert(possible_colors == _possible_colors[vertex]);
        }
        if (not _best_improve_colors.empty()) {
            assert(best_colors == _best_improve_colors[vertex]);
        }
        if (not _best_delta.empty()) {
            assert(min_delta == _best_delta[vertex]);
        }
    }
    assert(nb_uncolored == static_cast<int>(_uncolored.size()));
    return true;
}

std::vector<int> Solution::best_possible_colors(const int vertex) const {
    std::vector<int> best_colors;
    int min_conflicts = graph->nb_vertices;
    for (int color = 0; color < _nb_colors; ++color) {
        const int nb_conflicts = _conflicts[color][vertex];
        if (nb_conflicts > min_conflicts) {
            continue;
        }
        if (nb_conflicts < min_conflicts) {
            best_colors.clear();
            min_conflicts = nb_conflicts;
        }
        best_colors.emplace_back(color);
    }
    return best_colors;
}

bool Solution::is_legal() const {
    return _conflicting_vertices.empty() and _uncolored.empty() and _penalty == 0;
}

int Solution::nb_colors() const {
    return _nb_colors;
}

int Solution::operator[](const int vertex) const {
    return _colors[vertex];
}

int Solution::penalty() const {
    return _penalty;
}

const std::vector<int> &Solution::uncolored() const {
    return _uncolored;
}

int Solution::nb_uncolored() const {
    return static_cast<int>(_uncolored.size());
}

const std::vector<int> &Solution::conflicting_vertices() const {
    return _conflicting_vertices;
}

int Solution::best_delta(const int vertex) const {
    return _best_delta[vertex];
}

const std::vector<int> &Solution::best_improve_colors(const int vertex) const {
    return _best_improve_colors[vertex];
}

int Solution::delta_conflicts_colors(const int vertex, const int color) const {
    return _deltas[color][vertex];
}

int Solution::nb_conflicts(const int vertex) const {
    return _conflicts[_colors[vertex]][vertex];
}

int Solution::nb_conflicts(const int vertex, const int color) const {
    return _conflicts[color][vertex];
}

const std::vector<std::vector<int>> &Solution::conflicts_colors() const {
    return _conflicts;
}

int Solution::color_size(const int color) const {
    return static_cast<int>(_color_size[color]);
}

const std::vector<int> &Solution::possible_colors(const int vertex) const {
    return _possible_colors[vertex];
}

void Solution::to_legal() {
    while (not _conflicting_vertices.empty()) {
        int vertex = _conflicting_vertices.back();
        delete_from_color(vertex);
        std::vector<int> best_colors;
        for (int color = 0; color < _nb_colors; ++color) {
            if (_conflicts[color][vertex] == 0) {
                best_colors.emplace_back(color);
            }
        }
        if (best_colors.empty()) {
            best_colors.emplace_back(-1);
        }
        add_to_color(vertex, rd::choice(best_colors));
    }
}

const std::vector<int> &Solution::colors() const {
    return _colors;
}

int Solution::first_free_vertex() const {
    return _first_free_vertex;
}

void Solution::increment_first_free_vertex() {
    ++_first_free_vertex;
}

std::string Solution::format() const {
#ifdef DEBUG
    return fmt::format("{},{},{}",
                       fmt::styled(_uncolored.size(), fmt::bg(fmt::color::yellow_green)),
                       fmt::styled(_penalty, fmt::bg(fmt::color::red)),
                       fmt::styled(_nb_colors, fmt::bg(fmt::color::green)));
    // return fmt::format("{},{},{},{}",
    //                    fmt::styled(_uncolored.size(),
    //                    fmt::bg(fmt::color::yellow_green)),
    //                    fmt::styled(_penalty, fmt::bg(fmt::color::red)),
    //                    fmt::styled(_nb_colors, fmt::bg(fmt::color::green)),
    //                    fmt::join(_colors, ":"));
#else
    // return fmt::format("{},{},{}",
    //                    fmt::styled(_uncolored.size(),
    //                    fmt::bg(fmt::color::yellow_green)),
    //                    fmt::styled(_penalty, fmt::bg(fmt::color::red)),
    //                    fmt::styled(_nb_colors, fmt::bg(fmt::color::green)));
    // return fmt::format("{},{},{},{}",
    //                    fmt::styled(_uncolored.size(),
    //                    fmt::bg(fmt::color::yellow_green)),
    //                    fmt::styled(_penalty, fmt::bg(fmt::color::red)),
    //                    fmt::styled(_nb_colors, fmt::bg(fmt::color::green)),
    //                    fmt::join(_colors, ":"));
    return fmt::format(
        "{},{},{},{}", _uncolored.size(), _penalty, _nb_colors, fmt::join(_colors, ":"));
#endif
}

[[nodiscard]] int distance_approximation(const Solution &sol1, const Solution &sol2) {
    std::vector<std::vector<int>> same_color(parameters->nb_colors,
                                             std::vector<int>(parameters->nb_colors, 0));
    std::vector<int> maxi(parameters->nb_colors, 0);
    std::vector<int> sigma(parameters->nb_colors, 0);
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        ++same_color[sol1[vertex]][sol2[vertex]];
        if (same_color[sol1[vertex]][sol2[vertex]] > maxi[sol2[vertex]]) {
            maxi[sol1[vertex]] = same_color[sol1[vertex]][sol2[vertex]];
            sigma[sol1[vertex]] = sol2[vertex];
        }
    }
    int sum = 0;
    for (int color = 0; color < parameters->nb_colors; ++color) {
        sum += same_color[color][sigma[color]];
    }
    return graph->nb_vertices - sum;
}

[[nodiscard]] int distance_accurate(const Solution &sol1, const Solution &sol2) {
    // get the max number of colors
    const int max_k = parameters->nb_colors;

    // compute the number of same color in sol1 and sol2
    std::vector<std::vector<int>> same_color(max_k, std::vector<int>(max_k, 0));

    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int col1 = sol1[vertex];
        const int col2 = sol2[vertex];
        if (col1 != -1 and col2 != -1) {
            ++same_color[col1][col2];
        }
    }

    // find the corresponding color of sol1 in sol2
    std::vector<int> corresponding_color(max_k, 0);

    int distance = 0;

    for (int c = 0; c < max_k; ++c) {
        // find highest number of same color
        int max_val = -1;
        int max_c1 = -1;
        int max_c2 = -1;
        for (int c1 = 0; c1 < max_k; ++c1) {
            const auto max_element =
                std::max_element(same_color[c1].begin(), same_color[c1].end());
            const int max_val_tmp = *max_element;
            if (max_val_tmp > max_val) {
                max_val = max_val_tmp;
                max_c1 = c1;
                max_c2 =
                    static_cast<int>(std::distance(same_color[c1].begin(), max_element));
            }
        }
        // add the highest number to the corresponding color
        corresponding_color[max_c1] = max_c2;
        distance += max_val;

        // delete same color
        std::fill(same_color[max_c1].begin(), same_color[max_c1].end(), -1);
        for (int i = 0; i < max_k; ++i) {
            same_color[i][max_c2] = -1;
        }
    }

    return graph->nb_vertices - distance;
}

[[nodiscard]] int distance_approximation(const std::vector<int> &sol1,
                                         const std::vector<int> &sol2) {
    std::vector<std::vector<int>> same_color(parameters->nb_colors,
                                             std::vector<int>(parameters->nb_colors, 0));
    std::vector<int> maxi(parameters->nb_colors, 0);
    std::vector<int> sigma(parameters->nb_colors, 0);
    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        ++same_color[sol1[vertex]][sol2[vertex]];
        if (same_color[sol1[vertex]][sol2[vertex]] > maxi[sol2[vertex]]) {
            maxi[sol1[vertex]] = same_color[sol1[vertex]][sol2[vertex]];
            sigma[sol1[vertex]] = sol2[vertex];
        }
    }
    int sum = 0;
    for (int color = 0; color < parameters->nb_colors; ++color) {
        sum += same_color[color][sigma[color]];
    }
    return graph->nb_vertices - sum;
}

[[nodiscard]] int distance_accurate(const std::vector<int> &sol1,
                                    const std::vector<int> &sol2) {
    // get the max number of colors
    const int max_k = parameters->nb_colors;

    // compute the number of same color in sol1 and sol2
    std::vector<std::vector<int>> same_color(max_k, std::vector<int>(max_k, 0));

    for (int vertex = 0; vertex < graph->nb_vertices; ++vertex) {
        const int col1 = sol1[vertex];
        const int col2 = sol2[vertex];
        if (col1 != -1 and col2 != -1) {
            ++same_color[col1][col2];
        }
    }

    // find the corresponding color of sol1 in sol2
    std::vector<int> corresponding_color(max_k, 0);

    int distance = 0;

    for (int c = 0; c < max_k; ++c) {
        // find highest number of same color
        int max_val = -1;
        int max_c1 = -1;
        int max_c2 = -1;
        for (int c1 = 0; c1 < max_k; ++c1) {
            const auto max_element =
                std::max_element(same_color[c1].begin(), same_color[c1].end());
            const int max_val_tmp = *max_element;
            if (max_val_tmp > max_val) {
                max_val = max_val_tmp;
                max_c1 = c1;
                max_c2 =
                    static_cast<int>(std::distance(same_color[c1].begin(), max_element));
            }
        }
        // add the highest number to the corresponding color
        corresponding_color[max_c1] = max_c2;
        distance += max_val;

        // delete same color
        std::fill(same_color[max_c1].begin(), same_color[max_c1].end(), -1);
        for (int i = 0; i < max_k; ++i) {
            same_color[i][max_c2] = -1;
        }
    }

    return graph->nb_vertices - distance;
}
