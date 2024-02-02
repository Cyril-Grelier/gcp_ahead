#pragma once

#include "../representation/Parameters.hpp"
#include "../representation/Solution.hpp"
#include "LocalSearchAlgorithm.hpp"

struct SimulationHelper {
    SimulationHelper(int fit_condition_, int distance_min_, int depth_min_)
        : fit_condition(fit_condition_),
          distance_min(distance_min_),
          depth_min(depth_min_) {
    }

    int fit_condition;
    int distance_min;
    int depth_min;

    std::vector<std::vector<int>> past_solutions;
    std::vector<int> past_nb_colors;

    void accept_solution(const Solution &solution);

    bool distant_enough(const Solution &solution);
    bool score_low_enough(const Solution &solution);
    bool level_ok(const Solution &solution);
    bool depth_chance_ok(const Solution &solution);
};

/** @brief Pointer to simulation function*/
typedef bool (*simulation_ptr)(const Solution &, SimulationHelper &);

/**
 * @brief Get the simulation function
 *
 * @param simulation name of the simulation
 * @return simulation_ptr function
 */
simulation_ptr get_simulation_fct(const std::string &simulation);

/**
 * @brief Never allow to do a local search
 *
 * @param solution solution to check
 * @return bool always false
 */
bool no_ls(const Solution &solution, SimulationHelper &helper);

/**
 * @brief Always allow to do a local search
 *
 * @param solution solution to check
 * @return bool always true
 */
bool always_ls(const Solution &solution, SimulationHelper &helper);

/**
 * @brief Check if the fitness is low enough after the greedy to perform a local search
 *
 * @param solution solution to check
 * @return bool true if the local search have been run false otherwise
 */
bool fit(const Solution &solution, SimulationHelper &helper);

/**
 * @brief Allow to do a local search randomly with more chance if the solution is deep in
 * the three
 *
 * @param solution solution to check
 * @return bool true if the local search have been run false otherwise
 */
bool depth(const Solution &solution, SimulationHelper &helper);

/**
 * @brief Allow to do a local search only each nb_vertices / 10 level of the tree
 *
 * @param solution solution to check
 * @return bool true if the local search have been run false otherwise
 */
bool level(const Solution &solution, SimulationHelper &helper);

/**
 * @brief Allow to do a local search randomly with more chance if the solution is deep in
 * the three and if the fitness after greedy is low enough
 *
 * @param solution solution to check
 * @return bool true if the local search have been run false otherwise
 */
bool depth_fit(const Solution &solution, SimulationHelper &helper);
