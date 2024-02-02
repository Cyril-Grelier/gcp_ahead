#include "insertion.hpp"

#include <algorithm>

#include "../representation/Graph.hpp"
#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"

using namespace parameters_search;
using namespace graph_instance;

Insertion::Insertion(const insertion_ptr function_, const ParamInsertion &param_)
    : function(function_), param(param_) {
}

int Insertion::run(const std::vector<Solution> &population, const Solution &child) const {
    return function(population, child, param);
}

int insertion_best(const std::vector<Solution> &population,
                   const Solution &child,
                   const ParamInsertion &param) {
    (void)param; // unused parameter
    // the population is sorted by penalty
    // if the child is better than the worst individual of the population
    // then replace the worst individual
    if (child.penalty() < population.back().penalty()) {
        return static_cast<int>(population.size()) - 1;
    } else {
        return -1;
    }
}

int insertion_distance(const std::vector<Solution> &population,
                       const Solution &child,
                       const ParamInsertion &param) {
    auto pop_size = population.size();
    int min_dist_child = graph->nb_vertices;
    // find the min distances between each individuals
    std::vector<int> min_distance(param.population_size + 1);
    for (size_t i = 0; i < pop_size; ++i) {
        min_distance[i] = population[i].distances.at(child.id);
        if (min_distance[i] < min_dist_child) {
            min_dist_child = min_distance[i];
        }
        // find the min distance between the current individual and the population
        for (size_t j = 0; j < pop_size; ++j) {
            if (i == j) {
                continue;
            }
            const int dist = population[i].distances.at(population[j].id);
            if (dist < min_distance[i]) {
                min_distance[i] = dist;
            }
        }
    }
    min_distance[param.population_size] = min_dist_child;

    // compute for the insertion acceptance formula from
    // Z. Lü and J.-K. Hao, “A memetic algorithm for graph coloring,”
    // European Journal of Operational Research, vol. 203, no. 1, pp. 241–250,
    // May 2010, doi: 10.1016/j.ejor.2009.07.016.
    std::vector<double> score_distance(param.population_size + 1, 0);
    for (size_t i = 0; i < pop_size; ++i) {
        score_distance[i] = population[i].penalty() +
                            std::exp(0.05 * graph->nb_vertices /
                                     std::max(min_distance[i], graph->nb_vertices / 50));
        // if the solution is already in the population, give it a bad score
        if (min_distance[i] == 0) {
            score_distance[i] *= 10;
        }
    }
    score_distance[pop_size] =
        child.penalty() +
        std::exp(0.05 * graph->nb_vertices /
                 std::max(min_distance[pop_size], graph->nb_vertices / 50));
    // if the solution is already in the population, give it a bad score
    if (min_distance[pop_size] == 0) {
        score_distance[pop_size] *= 10;
    }

    // get the index of the individual with the worst score
    auto worst_index =
        static_cast<int>(std::max_element(score_distance.begin(), score_distance.end()) -
                         score_distance.begin());
    if (worst_index == static_cast<int>(pop_size)) {
        return -1;
    }
    return worst_index;
}
