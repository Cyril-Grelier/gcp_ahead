#include "selection.hpp"

#include <algorithm>
#include <tuple>

#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"

using namespace parameters_search;

Selection::Selection(const selection_ptr function_, const ParamSelection &param_)
    : function(function_), param(param_) {
}

std::vector<std::pair<int, int>>
Selection::run(const std::vector<Solution> &population) const {
    return function(population, param);
}

std::vector<std::pair<int, int>> selection_random(const std::vector<Solution> &population,
                                                  const ParamSelection &param) {
    std::vector<std::pair<int, int>> selected;
    selected.reserve(param.nb_selected);

    // select the fists parents randomly
    std::set<int> firsts_parents;
    std::uniform_int_distribution<int> dist(0, static_cast<int>(population.size()) - 1);

    while (static_cast<int>(firsts_parents.size()) != param.nb_selected) {
        firsts_parents.insert(dist(rd::generator));
    }

    // select the second parent as different from the first one
    for (const auto &first_parent : firsts_parents) {
        int second_parent;
        do {
            second_parent = dist(rd::generator);
        } while (first_parent == second_parent or population[first_parent].distances.at(
                                                      population[second_parent].id) == 0);
        selected.push_back(std::make_pair(first_parent, second_parent));
    }
    return selected;
}

std::vector<std::pair<int, int>>
selection_random_closest(const std::vector<Solution> &population,
                         const ParamSelection &param) {

    std::vector<std::pair<int, int>> selected;
    selected.reserve(param.nb_selected * 2);

    // select the fists parents randomly
    std::set<int> firsts_parents;
    std::uniform_int_distribution<int> dist(0, static_cast<int>(population.size()) - 1);

    while (static_cast<int>(firsts_parents.size()) != param.nb_selected) {
        firsts_parents.insert(dist(rd::generator));
    }

    // select the second parent as the closest individual of the first one
    for (const auto &first_parent : firsts_parents) {

        // closer parent selection
        std::vector<std::tuple<int, int>> seconds_parents;
        for (int second_parent_rank = 0;
             second_parent_rank < static_cast<int>(population.size());
             ++second_parent_rank) {
            if (first_parent == second_parent_rank) {
                continue;
            }
            const auto second_parent_id = population[second_parent_rank].id;
            seconds_parents.emplace_back(
                second_parent_rank,
                population[first_parent].distances.at(second_parent_id));
        }
        // sort the possible seconds parents per distance
        std::stable_sort(seconds_parents.begin(),
                         seconds_parents.end(),
                         [](const auto &a, const auto &b) {
                             return std::get<1>(a) < std::get<1>(b);
                         });
        // pick one of the 3 closest ones
        seconds_parents.resize(3);
        const auto second_parent = std::get<0>(rd::choice(seconds_parents));

        selected.push_back(std::make_pair(first_parent, second_parent));
    }
    return selected;
}

std::vector<std::pair<int, int>>
selection_elitist(const std::vector<Solution> &population, const ParamSelection &param) {
    std::vector<std::pair<int, int>> selected;
    selected.reserve(param.nb_selected);

    // select the first parents as the best individuals
    std::vector<int> firsts_parents;
    firsts_parents.reserve(param.nb_selected);
    for (int i = 0; i < param.nb_selected; ++i) {
        firsts_parents.push_back(i);
    }

    // select the second parent as different from the first one
    for (const auto &first_parent : firsts_parents) {
        int second_parent;
        for (int second_parent_rank = 0;
             second_parent_rank < static_cast<int>(population.size());
             ++second_parent_rank) {
            if (first_parent == second_parent_rank or
                population[first_parent].distances.at(
                    population[second_parent_rank].id) == 0) {
                continue;
            } else {
                second_parent = second_parent_rank;
                break;
            }
        }
        selected.push_back(std::make_pair(first_parent, second_parent));
    }
    return selected;
}

std::vector<std::pair<int, int>> selection_head(const std::vector<Solution> &population,
                                                const ParamSelection &param) {
    (void)population;
    (void)param;
    return {{0, 1}, {1, 0}};
}
