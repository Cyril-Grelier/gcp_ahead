#include "SimulationHelper.hpp"

#include "../representation/Graph.hpp"
#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

using namespace graph_instance;
using namespace parameters_search;

void SimulationHelper::accept_solution(const Solution &solution) {
    past_solutions.push_back(solution.colors());
    past_nb_colors.push_back(solution.nb_colors());
    fit_condition = std::min(solution.penalty(), fit_condition);
}

bool SimulationHelper::distant_enough(const Solution &solution) {
    bool distant_enough = true;
    for (size_t i = 0; i < past_solutions.size() and distant_enough; ++i) {
        const int dist{distance_approximation(past_solutions[i], solution.colors())};
        if (dist < distance_min) {
            return false;
        }
    }
    return true;
}

bool SimulationHelper::score_low_enough(const Solution &solution) {
    const int min_score =
        std::max(static_cast<int>(fit_condition * 1.01), fit_condition + 1);
    const bool score_low_enough = solution.penalty() <= min_score;
    return score_low_enough;
}

bool SimulationHelper::level_ok(const Solution &solution) {
    const int v = solution.first_free_vertex();
    const int level = v % depth_min;
    const bool level_ok = v > depth_min and (level == 0);
    return level_ok;
}

bool SimulationHelper::depth_chance_ok(const Solution &solution) {
    // the local search can only be launch between 5 and 90 % of the tree
    std::uniform_int_distribution<int> distribution(5, 95);
    const int percentage_already_colored{(solution.first_free_vertex() * 100) /
                                         graph->nb_vertices};
    const int chance_of_passing = distribution(rd::generator);
    const bool depth_chance_ok = percentage_already_colored >= chance_of_passing;
    return depth_chance_ok;
}

bool no_ls(const Solution &solution, SimulationHelper &helper) {
    (void)solution;
    (void)helper;
    return false;
}

bool always_ls(const Solution &solution, SimulationHelper &helper) {
    (void)solution;
    (void)helper;
    return true;
}

bool fit(const Solution &solution, SimulationHelper &helper) {
    if (not helper.score_low_enough(solution)) {
        return false;
    }

    if (not helper.distant_enough(solution)) {
        return false;
    }

    helper.accept_solution(solution);
    return true;
}

bool depth(const Solution &solution, SimulationHelper &helper) {

    if (not helper.depth_chance_ok(solution)) {
        return false;
    }

    if (not helper.distant_enough(solution)) {
        return false;
    }

    helper.accept_solution(solution);
    return true;
}

bool level(const Solution &solution, SimulationHelper &helper) {

    if (not helper.level_ok(solution)) {
        return false;
    }

    if (not helper.distant_enough(solution)) {
        return false;
    }

    helper.accept_solution(solution);
    return true;
}

bool depth_fit(const Solution &solution, SimulationHelper &helper) {
    if (not helper.score_low_enough(solution)) {
        return false;
    }

    if (not helper.depth_chance_ok(solution)) {
        return false;
    }

    if (not helper.distant_enough(solution)) {
        return false;
    }

    helper.accept_solution(solution);
    return true;
}
