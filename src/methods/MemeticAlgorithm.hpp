#pragma once

#include <tuple>

#include "../representation/Method.hpp"
#include "../representation/Solution.hpp"
#include "GreedyAlgorithm.hpp"
#include "LocalSearchAlgorithm.hpp"
#include "adaptive.hpp"
#include "crossover.hpp"
#include "insertion.hpp"
#include "selection.hpp"

struct ParamMA {
    const std::string name;
    int population_size;
    int nb_selected;
    const int max_time;
    const long max_iterations;
    Selection selection;
    std::vector<Crossover> crossover;
    std::vector<LocalSearch> local_search;
    Insertion insertion;
    std::vector<std::tuple<int, int>> pairs_x_ls;
    std::vector<std::string> x_ls_names;
    std::shared_ptr<AdaptiveHelper> adaptive_helper;
    long elites;
};

/**
 * @brief Method for Memetic algorithm
 *
 */
class MemeticAlgorithm : public Method {

    /** @brief Best found solution*/
    Solution _best_solution;

    const ParamMA _param;

    /** @brief Init function*/
    greedy_fct_ptr _greedy_function;

    /** @brief Current population*/
    std::vector<Solution> _population;

    /** @brief Selected solutions*/
    std::vector<std::pair<int, int>> _selected;
    /** @brief Children solutions*/
    std::vector<Solution> _children;
    /** @brief Time before founding best score*/
    std::chrono::high_resolution_clock::time_point _t_best;
    /** @brief Current turn of search*/
    long _turn = 0;
    /** @brief Functions of pairs of operators to call*/
    std::vector<std::tuple<crossover_ptr, int, local_search_ptr>>
        _crossover_and_local_search;
    /** @brief Output file name if not on console*/
    std::string output_file_tbt;
    /** @brief Output for turn by turn info, stdout default else output_directory/tbt */
    std::FILE *output_tbt = nullptr;

  public:
    explicit MemeticAlgorithm(greedy_fct_ptr greedy_function_, const ParamMA &param_);

    ~MemeticAlgorithm();

    /**
     * @brief Stopping condition for the MemeticAlgorithm depending on turn or time limit
     *
     * @return true continue the search
     * @return false stop the search
     */
    bool stop_condition() const;

    /**
     * @brief Run function for the method
     */
    void run() override;

    std::string crossover_and_mutation();

    std::tuple<std::string, std::string> crossover_and_mutation_neural_network();

    void insertion_head();

    void insert(Solution &child);

    /**
     * @brief Insertion of the children solutions in the population
     *
     */
    void insertion();

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

    void update_best_score();
    void turn_by_turn_line(const std::string &selected_indiv_str,
                           const std::string &selected_distance_str,
                           const std::string &fit_str,
                           const std::string &selected_str);
};
