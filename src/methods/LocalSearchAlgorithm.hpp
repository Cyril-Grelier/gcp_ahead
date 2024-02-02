#pragma once

#include <optional>

#include "../representation/Method.hpp"
#include "../representation/Solution.hpp"
#include "GreedyAlgorithm.hpp"

struct ParamLS {
    const std::string name;
    const std::string pseudo;
    const double alpha;
    const int random_min;
    const int random_max;
    const int max_time;
    const long max_iterations;
    const bool verbose;
};

/** @brief Pointer to local search function */
typedef std::optional<Solution> (*local_search_ptr)(Solution &, const ParamLS &);
// typedef std::optional<Solution> (*local_search_ptr)(Solution &, bool);

struct LocalSearch {
    const local_search_ptr function;
    const ParamLS param;

    explicit LocalSearch(const local_search_ptr function_, const ParamLS &parameters_);

    std::optional<Solution> run(Solution &solution) const;
};

/**
 * @brief Method for local search
 */
class LocalSearchAlgorithm : public Method {

    /** @brief Best found solution*/
    Solution _best_solution;

    /** @brief greedy function*/
    greedy_fct_ptr _greedy_function;

    /** @brief Local search function*/
    LocalSearch _local_search;

  public:
    explicit LocalSearchAlgorithm(greedy_fct_ptr greedy_function_,
                                  const LocalSearch &local_search);

    ~LocalSearchAlgorithm() override = default;

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
 * @brief Get the local search
 *
 * @param local_search
 * @return local_search_ptr function local search
 */
local_search_ptr get_local_search_fct(const std::string &local_search);

/**
 * @brief partial_col
 *
 */
std::optional<Solution> partial_col(Solution &solution, const ParamLS &param);

std::optional<Solution> partial_col_optimized(Solution &best_solution,
                                              const ParamLS &param);

/**
 * @brief partial_ts inspired from ILS-TS
 *
 */
std::optional<Solution> partial_ts(Solution &solution, const ParamLS &param);

/**
 * @brief TabuCol
 *
 * From :
 * Hertz, A., Werra, D., 1987.
 * Werra, D.: Using Tabu Search Techniques for Graph Coloring.
 * Computing 39, 345-351. Computing 39.
 * https://doi.org/10.1007/BF02239976
 *
 */
std::optional<Solution> tabu_col(Solution &solution, const ParamLS &param);

/**
 * @brief TabuCol optimized
 *
 * Optimizations from :
 * Moalic, Laurent, and Alexandre Gondran.
 * Variations on Memetic Algorithms for Graph Coloring Problems.
 * Journal of Heuristics 24, no. 1 (February 2018): 1–24.
 * https://doi.org/10.1007/s10732-017-9354-9.
 *
 * Based on :
 * Hertz, A., Werra, D., 1987.
 * Werra, D.: Using Tabu Search Techniques for Graph Coloring.
 * Computing 39, 345-351. Computing 39.
 * https://doi.org/10.1007/BF02239976
 *
 */
std::optional<Solution> tabu_col_optimized(Solution &solution, const ParamLS &param);

/**
 * @brief Tabu bucket
 *
 * Works with on the UBQP version of the problem
 *
 */
std::optional<Solution> tabu_bucket(Solution &solution, const ParamLS &param);
