#include "adaptive.hpp"

#include <algorithm>
#include <cmath>

#include "../representation/Graph.hpp"
#include "../utils/random_generator.hpp"
#include "../utils/utils.hpp"

using namespace graph_instance;
using namespace parameters_search;

AdaptiveHelper::AdaptiveHelper(const ParamAdapt &param_)
    : param(param_),
      proba_operator(param_.nb_operators, 1 / static_cast<double>(param_.nb_operators)),
      utility(param_.memory_size, 0),
      past_operators(param_.memory_size, -1),
      normalized_utilities(param_.nb_operators, 0),
      nb_times_selected(param_.nb_operators, 0),
      nb_times_used_total(param_.nb_operators, 0),
      mean_score(param_.nb_operators, 0),
      possible_operators() {
    for (int i = 0; i < param_.nb_operators; ++i) {
        possible_operators.insert(i);
    }
}

void AdaptiveHelper::update_obtained_solution(const int index_solution,
                                              const int operator_number,
                                              const int score) {
    const int index = ((turn * param.nb_selected) % param.memory_size + index_solution);
    utility[index] = score;
    past_operators[index] = operator_number;
    mean_score[operator_number] =
        ((mean_score[operator_number] *
          static_cast<double>(nb_times_used_total[operator_number])) +
         score) /
        static_cast<double>(nb_times_used_total[operator_number] + 1);
    ++nb_times_used_total[operator_number];
}

void AdaptiveHelper::update_helper() {
}

void AdaptiveHelper::increment_turn() {
    ++turn;
}

std::string AdaptiveHelper::to_str_proba() const {
    return fmt::format("{:.2f}", fmt::join(proba_operator, ":"));
}

void AdaptiveHelper::compute_normalized_utilities_and_nb_selected() {
    std::fill(normalized_utilities.begin(), normalized_utilities.end(), 0);
    std::fill(nb_times_selected.begin(), nb_times_selected.end(), 0);

    // get the sum of past utilities
    for (size_t i = 0; i < utility.size(); ++i) {
        const int operator_selected = past_operators[i];
        if (operator_selected != -1) {
            ++nb_times_selected[operator_selected];
            normalized_utilities[operator_selected] += utility[i];
        }
    }

    // mean the utilities
    for (int o = 0; o < param.nb_operators; ++o) {
        if (nb_times_selected[o] != 0) {
            normalized_utilities[o] = normalized_utilities[o] / nb_times_selected[o];
        }
    }
    // if an operator have never been selected,
    // then its utility is the one of the worst operator
    const double worst{
        *std::max_element(normalized_utilities.begin(), normalized_utilities.end())};
    for (int o = 0; o < param.nb_operators; ++o) {
        if (nb_times_selected[o] == 0) {
            normalized_utilities[o] = worst;
        }
    }
    // normalization
    const auto minmax{
        std::minmax_element(normalized_utilities.begin(), normalized_utilities.end())};
    const double min_val = *minmax.first;
    const double max_val = *minmax.second;
    if (min_val == max_val) {
        // if all the operators have the same mean score
        // they all get normalized to 1
        for (int o = 0; o < param.nb_operators; ++o) {
            normalized_utilities[o] = 1;
        }
    } else {
        // otherwise the utilities are normally normalized
        // in reverse as its a minimization problem
        for (int o = 0; o < param.nb_operators; ++o) {
            normalized_utilities[o] =
                (normalized_utilities[o] - max_val) / (min_val - max_val);
        }
    }
}

std::string AdaptiveHelper::get_selected_str() const {
    const int index = (turn * param.nb_selected) % param.memory_size;
    std::string result = "";
    for (int i = 0; i < param.nb_selected; ++i) {
        result += std::to_string(past_operators[index + i]) +
                  (i == (param.nb_selected - 1) ? "" : ":");
    }
    return result;
}

/************************************************************************
 *
 *                         AdaptiveHelper_none
 *
 ************************************************************************/

AdaptiveHelper_none::AdaptiveHelper_none(const ParamAdapt &param_)
    : AdaptiveHelper(param_) {
}

int AdaptiveHelper_none::get_operator() {
    return 0;
}

/************************************************************************
 *
 *                         AdaptiveHelper_iterated
 *
 ************************************************************************/

AdaptiveHelper_iterated::AdaptiveHelper_iterated(const ParamAdapt &param_)
    : AdaptiveHelper(param_) {
}

int AdaptiveHelper_iterated::get_operator() {
    const int operator_number = turn % param.nb_operators;
    return operator_number;
}

/************************************************************************
 *
 *                         AdaptiveHelper_random
 *
 ************************************************************************/

AdaptiveHelper_random::AdaptiveHelper_random(const ParamAdapt &param_)
    : AdaptiveHelper(param_) {
}

int AdaptiveHelper_random::get_operator() {
    std::discrete_distribution<> d(proba_operator.begin(), proba_operator.end());
    const int operator_number = d(rd::generator);
    return operator_number;
}

/************************************************************************
 *
 *                         AdaptiveHelper_deleter
 *
 ************************************************************************/

AdaptiveHelper_deleter::AdaptiveHelper_deleter(const ParamAdapt &param_)
    : AdaptiveHelper(param_) {
}

int AdaptiveHelper_deleter::get_operator() {
    const int operator_number = rd::choice(possible_operators);
    return operator_number;
}

void AdaptiveHelper_deleter::update_helper() {
    if ((turn < 5 * param.nb_operators) or (not(turn % 5 == 0)) or
        (possible_operators.size() == 1)) {
        return;
    }
    int worst_operator = *possible_operators.begin();
    for (const auto o : possible_operators) {
        if (mean_score[o] > mean_score[worst_operator]) {
            worst_operator = o;
        }
    }
    possible_operators.erase(worst_operator);
    removed_operators.insert(worst_operator);
}

/************************************************************************
 *
 *                         AdaptiveHelper_roulette_wheel
 *
 ************************************************************************/

AdaptiveHelper_roulette_wheel::AdaptiveHelper_roulette_wheel(const ParamAdapt &param_)
    : AdaptiveHelper(param_) {
}

int AdaptiveHelper_roulette_wheel::get_operator() {
    std::discrete_distribution<> d(proba_operator.begin(), proba_operator.end());
    const int operator_number = d(rd::generator);
    return operator_number;
}

void AdaptiveHelper_roulette_wheel::update_helper() {
    if (turn < 5 * param.nb_operators) {
        return;
    }

    compute_normalized_utilities_and_nb_selected();

    double sum_uk = sum(normalized_utilities);

    const double p_min = 1.0 / static_cast<double>(param.nb_operators * 5);
    for (auto o = 0; o < param.nb_operators; ++o) {
        proba_operator[o] =
            p_min + (1 - param.nb_operators * p_min) * (normalized_utilities[o] / sum_uk);
    }
}

/************************************************************************
 *
 *                         AdaptiveHelper_pursuit
 *
 ************************************************************************/

AdaptiveHelper_pursuit::AdaptiveHelper_pursuit(const ParamAdapt &param_)
    : AdaptiveHelper(param_) {
}

int AdaptiveHelper_pursuit::get_operator() {
    std::discrete_distribution<> d(proba_operator.begin(), proba_operator.end());
    const int operator_number = d(rd::generator);
    return operator_number;
}

void AdaptiveHelper_pursuit::update_helper() {
    if (turn < 5 * param.nb_operators) {
        return;
    }

    if (turn % 20 == 0) {
        // reset proba
        const double val = 1 / static_cast<double>(param.nb_operators);
        std::fill(proba_operator.begin(), proba_operator.end(), val);
    }

    const double p_min = 1.0 / static_cast<double>(param.nb_operators * 5);
    const double p_max = 1 - static_cast<double>(param.nb_operators - 1) * p_min;
    const double beta = 0.7;

    compute_normalized_utilities_and_nb_selected();

    for (int o = 0; o < param.nb_operators; ++o) {
        const double t_minus_1 = proba_operator[o];
        if (normalized_utilities[o] == 1) {
            // if its the best operator (normalization at 1)
            proba_operator[o] = t_minus_1 + beta * (p_max - t_minus_1);
        } else {
            proba_operator[o] = t_minus_1 + beta * (p_min - t_minus_1);
        }
    }
}

/************************************************************************
 *
 *                         AdaptiveHelper_ucb
 *
 ************************************************************************/

AdaptiveHelper_ucb::AdaptiveHelper_ucb(const ParamAdapt &param_)
    : AdaptiveHelper(param_) {
}

int AdaptiveHelper_ucb::get_operator() {
    const double maxi = *std::max_element(proba_operator.begin(), proba_operator.end());
    std::vector<int> bests;
    for (int o = 0; o < param.nb_operators; ++o) {
        if (proba_operator[o] == maxi) {
            bests.push_back(o);
        }
    }
    const int operator_number = rd::choice(bests);
    return operator_number;
}

void AdaptiveHelper_ucb::update_helper() {
    if (turn < 5 * param.nb_operators) {
        return;
    }
    compute_normalized_utilities_and_nb_selected();

    int size = param.memory_size;
    if (turn < param.memory_size) {
        size = (turn + 1) * param.nb_selected;
    }

    // Exploration
    std::vector<double> exploration(param.nb_operators, 0);
    for (int o = 0; o < param.nb_operators; ++o) {
        exploration[o] = std::sqrt(2 * std::log(size) / (nb_times_selected[o] + 1));
    }
    // set the ucb score
    for (int o = 0; o < param.nb_operators; ++o) {
        proba_operator[o] =
            normalized_utilities[o] + param.coeff_exploi_explo * exploration[o];
    }
}

/************************************************************************
 *
 *                         AdaptiveHelper_neural_net
 *
 ************************************************************************/

AdaptiveHelper_neural_net::AdaptiveHelper_neural_net(const ParamAdapt &param_)
    : AdaptiveHelper(param_),
      _model(parameters->nb_colors, graph->nb_vertices, param_.nb_operators),
      _optimizer(_model.parameters(), /*lr=*/0.001) {
    _solutions.reserve(param.memory_size);
    _utility_operator.reserve(param.memory_size);
}

int AdaptiveHelper_neural_net::get_operator() {
    fmt::print(stderr, "error use get_operator_nn for the neural net operator");
    throw 1;
}

int AdaptiveHelper_neural_net::get_operator(const int index_solution,
                                            const Solution &solution) {
    auto tensor_solution = solution_to_tensor(solution);
    if (turn < (param.memory_size / param.nb_selected)) {
        _solutions.emplace_back(tensor_solution);
        _utility_operator.emplace_back(torch::tensor({-1, -1}));
    } else {
        const int index = (turn * param.nb_selected) % param.memory_size + index_solution;
        _solutions[index] = tensor_solution;
        _utility_operator[index] = torch::tensor({-1, -1});
    }

    int operator_number;

    std::uniform_int_distribution<int> distribution(0, 99);
    if (distribution(rd::generator) < 20 or turn < 5 * param.nb_operators) {
        std::uniform_int_distribution<int> dist(
            0, static_cast<int>(proba_operator.size() - 1));
        operator_number = dist(rd::generator);
    } else {
        torch::Tensor prediction = _model.forward(tensor_solution.unsqueeze(0));
        if (param.nb_selected == 1) {
            // update the proba with the predictions
            for (int op = 0; op < param.nb_operators; ++op) {
                proba_operator[op] = -prediction.index({0, op}).item().toDouble();
            }
        }
        operator_number = prediction.argmin(1).item().toInt();
    }
    return operator_number;
}

void AdaptiveHelper_neural_net::update_obtained_solution(const int index_solution,
                                                         const int operator_number,
                                                         const int score) {
    AdaptiveHelper::update_obtained_solution(index_solution, operator_number, score);
    const int index = (turn * param.nb_selected) % param.memory_size + index_solution;
    _utility_operator[index] = torch::tensor(
        {static_cast<long>(utility[index]), static_cast<long>(past_operators[index])});
}

void AdaptiveHelper_neural_net::update_helper() {
    if (turn < 5 * param.nb_operators or turn % 20 != 0) {
        return;
    }
    // train the NN
    auto data_set = SolutionDataset(_solutions, _utility_operator)
                        .map(torch::data::transforms::Stack<>());
    auto batch_size = 20;
    auto data_loader =
        torch::data::make_data_loader<torch::data::samplers::RandomSampler>(
            std::move(data_set), batch_size);
    const int nb_epoch = 10;
    train(data_loader, _model, _optimizer, nb_epoch);
}

int AdaptiveHelper_neural_net::select_best(const std::vector<Solution> &children) {
    int selected;

    // chance of random selection
    std::uniform_int_distribution<int> distribution(0, 99);
    if (distribution(rd::generator) < 20 or turn < 10) {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(children.size()) - 1);
        selected = dist(rd::generator);
        return selected;
    }

    int best_score = std::numeric_limits<int>::max();
    std::vector<int> best_sols;
    for (size_t i = 0; i < children.size(); ++i) {
        auto tensor_child = solution_to_tensor(children[i]);
        // get the prediction for each possible operator
        auto predictions = _model.predict(tensor_child.unsqueeze(0));
        // find the lowest prediction
        const int min_score = *std::min_element(predictions.begin(), predictions.end());
        if (min_score < best_score) {
            best_score = min_score;
            best_sols.clear();
        }
        if (min_score == best_score) {
            best_sols.emplace_back(i);
        }
    }

    selected = rd::choice(best_sols);
    return selected;
}
