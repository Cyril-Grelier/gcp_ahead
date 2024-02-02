#include "MCTS.hpp"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <utility>

#include "../representation/Graph.hpp"
#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"
#include "../utils/utils.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

using namespace graph_instance;
using namespace parameters_search;

MCTS::MCTS(greedy_fct_ptr greedy_function_, const ParamMCTS &param_)
    : _param(param_),
      _root_node(nullptr),
      _current_node(_root_node),
      _base_solution(),
      _best_solution(),
      _current_solution(_base_solution),
      _turn(0),
      _greedy_function(greedy_function_) {
    _greedy_function(_best_solution);
    _t_best = std::chrono::high_resolution_clock::now();

    if (not parameters->use_target) {
        Solution::best_nb_colors = _best_solution.nb_colors();
    }

    // Creation of the base solution and root node
    const auto next_moves = next_possible_moves(_base_solution);
    assert(next_moves.size() == 1);
    apply_action(_base_solution, next_moves[0]);
    const auto next_possible_actions = next_possible_moves(_base_solution);
    _root_node = std::make_shared<Node>(nullptr, next_moves[0], next_possible_actions);
    _current_node = _root_node;
    fmt::print(parameters->output, "{}", header_csv());

    fmt::print(parameters->output, "{}", line_csv());
    // Prepare adaptive helper
    // get list of operators

    if (parameters->output_file != "") {
        output_file_tbt = fmt::format("{}/tbt/{}_{}_{}.csv",
                                      parameters->output_directory,
                                      graph->name,
                                      parameters->rand_seed,
                                      parameters->nb_colors);

        std::FILE *file_tbt = std::fopen((output_file_tbt + ".running").c_str(), "w");
        if (!file_tbt) {
            throw std::runtime_error(
                fmt::format("error while trying to access {}\n", output_file_tbt));
        }
        output_tbt = file_tbt;
    } else {
        output_tbt = stdout;
    }

    if (_param.local_search.size() > 0) {
        fmt::print(output_tbt, "#operators\n");
        fmt::print(output_tbt, "#{}\n", fmt::join(_param.ls_names, ":"));

        fmt::print(output_tbt, "time,turn,proba,selected,score_pre_ls,score_post_ls\n");
    }
}

MCTS::~MCTS() {
    _current_node = nullptr;
    _root_node->clean_graph(0);
    _root_node = nullptr;
}

bool MCTS::stop_condition() const {
    return (_turn < _param.max_iterations) and (not parameters->time_limit_reached()) and
           not(parameters->use_target and
               (_best_solution.nb_colors() == parameters->nb_colors and
                _best_solution.is_legal())) and
           not _root_node->fully_explored();
}

void MCTS::run() {
    SimulationHelper helper(Solution::best_nb_colors,
                            std::max(graph->nb_vertices / 10, 3),
                            std::max(graph->nb_vertices / 5, 3));
    int operator_number = 0;
    auto *cast_nn =
        dynamic_cast<AdaptiveHelper_neural_net *>(_param.adaptive_helper.get());

    while (stop_condition()) {
        ++_turn;

        _current_node = _root_node;
        _current_solution = _base_solution;

        selection();

        expansion();

        // simulation
        _greedy_function(_current_solution);
        const int score_before_ls = _current_solution.nb_colors();

        // local search or not and adaptive selection
        const bool use_local_search = _param._simulation(_current_solution, helper);
        if (use_local_search) {
            // ask the adaptive helper which local search to use
            if (cast_nn) {
                operator_number = cast_nn->get_operator(0, _current_solution);
            } else {
                operator_number = _param.adaptive_helper->get_operator();
            }
            const auto ls = _param.local_search[operator_number];
            auto last_legal = ls.run(_current_solution);
            if (last_legal) {
                _current_solution = last_legal.value();
            } else {
                fmt::print(stderr, "LS {} failed\n", ls.param.pseudo);
            }

            _param.adaptive_helper->update_obtained_solution(
                0, operator_number, _current_solution.nb_colors());
            _param.adaptive_helper->update_helper();
            fmt::print(
                output_tbt,
                "{},{},{},{},{},{}\n",
                parameters->elapsed_time(std::chrono::high_resolution_clock::now()),
                _turn,
                _param.adaptive_helper->to_str_proba(),
                operator_number,
                score_before_ls,
                _current_solution.nb_colors());

            _param.adaptive_helper->increment_turn();
        }

        const int nb_colors = _current_solution.nb_colors();
        // update
        _current_node->update(nb_colors);
        // update and print best score
        if (_best_solution.nb_colors() > nb_colors) {
            _t_best = std::chrono::high_resolution_clock::now();
            _best_solution = _current_solution;
            if (Solution::best_nb_colors > nb_colors)
                Solution::best_nb_colors = nb_colors;
            fmt::print(parameters->output, "{}", line_csv());
            _current_node = nullptr;
            _root_node->clean_graph(_best_solution.nb_colors());
        }
        _current_node = nullptr;
    }
    _current_node = _root_node;
    fmt::print(parameters->output, "{}", line_csv());
    _current_node = nullptr;
}

void MCTS::selection() {
    while (not _current_node->terminal()) {
        double max_score = std::numeric_limits<double>::min();
        std::vector<std::shared_ptr<Node>> next_nodes;
        for (const auto &node : _current_node->children_nodes()) {
            if (node->score_ucb() > max_score) {
                max_score = node->score_ucb();
                next_nodes = {node};
            } else if (node->score_ucb() == max_score) {
                next_nodes.push_back(node);
            }
        }
        _current_node = rd::choice(next_nodes);
        apply_action(_current_solution, _current_node->move());
    }
}

void MCTS::expansion() {
    const Action next_move = _current_node->next_child();
    apply_action(_current_solution, next_move);
    const auto next_possible_actions = next_possible_moves(_current_solution);
    if (not next_possible_actions.empty()) {
        _current_node =
            std::make_shared<Node>(_current_node.get(), next_move, next_possible_actions);
        _current_node->add_child_to_parent(_current_node);
    }
}

[[nodiscard]] const std::string MCTS::header_csv() const {
    return fmt::format("turn,time,depth,nb total node,nb "
                       "current node,height,{}\n",
                       Solution::header_csv);
}

[[nodiscard]] const std::string MCTS::line_csv() const {
    return fmt::format("{},{},{},{},{},{},{}\n",
                       _turn,
                       parameters->elapsed_time(_t_best),
                       _current_node->get_depth(),
                       Node::get_total_nodes(),
                       Node::get_nb_current_nodes(),
                       Node::get_height(),
                       _best_solution.format());
}

std::vector<Action> next_possible_moves(const Solution &solution) {
    std::vector<Action> moves;
    const int next_vertex = solution.first_free_vertex();
    if (next_vertex == graph->nb_vertices) {
        return moves;
    }

    int nb_colors = 0;
    // each vertex need at most |N(v)| + 1 colors
    int degree_p1 = static_cast<int>(graph->neighborhood[next_vertex].size()) + 1;

    for (int color = 0; color < solution.nb_colors(); ++color) {
        ++nb_colors;
        if (nb_colors > degree_p1) {
            continue;
        }
        if (solution.nb_conflicts(next_vertex, color) == 0) {
            moves.emplace_back(Action{next_vertex, color, solution.nb_colors()});
        }
    }
    const int next_score = solution.nb_colors() + 1;
    if (Solution::best_nb_colors > next_score) {
        if (nb_colors < degree_p1) {
            moves.emplace_back(Action{next_vertex, -1, next_score});
        }
    }
    std::sort(moves.begin(), moves.end(), compare_actions);
    return moves;
}

void apply_action(Solution &solution, const Action &action) {
    solution.add_to_color(action.vertex, action.color);
    assert(solution.first_free_vertex() == action.vertex);
    solution.increment_first_free_vertex();
    assert(solution.nb_colors() == action.score);
}

void MCTS::to_dot(const std::string &file_name) const {
    if ((_turn % 5) == 0) {
        std::ofstream file{file_name};
        file << _root_node->to_dot();
        file.close();
    }
}
