#pragma once

#include "../representation/Solution.hpp"

struct ParamCrossover {
    std::string name;
    std::string pseudo;
    int percentage_p1;
    int colors_p1;
};

typedef void (*crossover_ptr)(const Solution &,
                              const Solution &,
                              Solution &,
                              const ParamCrossover &);

struct Crossover {
    const crossover_ptr function;
    const ParamCrossover param;

    explicit Crossover(const crossover_ptr function_, const ParamCrossover &parameters_);

    void run(const Solution &parent1, const Solution &parent2, Solution &child) const;
};

/**
 * @brief Recreate parent1 (does nothing with parent2) in the child
 *
 * @param parent1 first parent (used)
 * @param parent2 second parent (unused)
 * @param child created child
 */
void no_crossover(const Solution &parent1,
                  const Solution &parent2,
                  Solution &child,
                  const ParamCrossover &param);

/**
 * @brief Create a child by alternating colors from parent1 and parent2, using bigger
 * colors first, takes colors_p1 colors from parent1 for 1 color from parent2
 *
 * From :
 * Galinier, P., Hao, J.-K., 1999.
 * Hybrid Evolutionary Algorithms for Graph Coloring.
 * Journal of Combinatorial Optimization 3, 379–397.
 * https://doi.org/10.1023/A:1009823419804
 *
 * @param parent1 first parent
 * @param parent2 second parent
 * @param child created child
 * @param param parameters
 */
void gpx(const Solution &parent1,
         const Solution &parent2,
         Solution &child,
         const ParamCrossover &param);

/**
 * @brief Create a child by using percentage_p1 of parent1 colors then alternating between
 * parent1 and parent2
 *
 * @param parent1 first parent
 * @param parent2 second parent
 * @param child created child
 * @param param parameters
 */
void partial_best_gpx(const Solution &parent1,
                      const Solution &parent2,
                      Solution &child,
                      const ParamCrossover &param);

void partial_random_gpx(const Solution &parent1,
                        const Solution &parent2,
                        Solution &child,
                        const ParamCrossover &param);
