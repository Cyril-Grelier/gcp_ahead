#pragma once

#include "../representation/Method.hpp"
#include "../representation/Solution.hpp"

/** @brief Pointer to initialization function*/
typedef void (*greedy_fct_ptr)(Solution &solution);

/**
 * @brief Method for local search
 */
class GreedyAlgorithm : public Method {

    /** @brief Best found solution*/
    Solution _best_solution;

    /** @brief greedy function*/
    greedy_fct_ptr _greedy_function;

  public:
    explicit GreedyAlgorithm(greedy_fct_ptr greedy_function_);

    ~GreedyAlgorithm() override = default;

    /**
     * @brief Run function for the method
     */
    void run() override;

    /**
     * @brief Return method header in csv format
     *
     * @return std::string method header in csv format
     */
    [[nodiscard]] const std::string header_csv() const override;

    /**
     * @brief Return method in csv format
     *
     * @return std::string method in csv format
     */
    [[nodiscard]] const std::string line_csv() const override;
};

/**
 * @brief Greedy algorithm, that color vertices with random color
 * give a chance to open a new color for each vertex
 *
 */
void greedy_random(Solution &solution);

/**
 * @brief Greedy algorithm, that color vertices with random color
 * give a chance to open a new color only if needed
 *
 */
void greedy_constrained(Solution &solution);

/**
 * @brief Greedy algorithm, that color vertices with first available color
 *
 */
void greedy_deterministic(Solution &solution);

/**
 * @brief Greedy algorithm, that add to each color every possible vertices
 * should be the same as greedy_deterministic
 *
 */
void greedy_deterministic_2(Solution &solution);

/**
 * @brief Greedy algorithm, that add to each color every possible vertices
 * adapt the order of vertices to color with the number of colored neighbors
 *
 */
void greedy_adaptive(Solution &solution);

/**
 * @brief Greedy algorithm, color vertices by saturation order
 *
 * code adapted from :
 * R. Lewis
 * Guide to Graph Colouring: Algorithms and Applications.
 * 2021. doi: 10.1007/978-3-030-81054-2.
 *
 */
void greedy_DSatur(Solution &solution);

/**
 * @brief Greedy algorithm, color vertices by independent set blocks
 *
 * code adapted from :
 * R. Lewis
 * Guide to Graph Colouring: Algorithms and Applications.
 * 2021. doi: 10.1007/978-3-030-81054-2.
 *
 */
void greedy_RLF(Solution &solution);

/**
 * @brief Get the greedy function
 */
greedy_fct_ptr get_greedy_fct(const std::string &initialization);
