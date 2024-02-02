#pragma once

#include "../representation/Solution.hpp"

struct ParamInsertion {
    std::string name;
    int population_size;
};

/**
 * @brief Pointer to insertion function
 *
 * the function take in parameter the population, the child and the parameters for the
 * insertion and return the index in the population of the individuals that will be
 * replaced by the child. -1 means that the child is not inserted in the population.
 * The population must be sorted by penalty.
 *
 */
typedef int (*insertion_ptr)(const std::vector<Solution> &,
                             const Solution &,
                             const ParamInsertion &);

struct Insertion {
    const insertion_ptr function;
    const ParamInsertion param;

    explicit Insertion(const insertion_ptr function_, const ParamInsertion &param_);

    int run(const std::vector<Solution> &population, const Solution &child) const;
};

int insertion_best(const std::vector<Solution> &population,
                   const Solution &child,
                   const ParamInsertion &param);

int insertion_distance(const std::vector<Solution> &population,
                       const Solution &child,
                       const ParamInsertion &param);
