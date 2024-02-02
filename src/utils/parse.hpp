#pragma once

#include <nlohmann/json.hpp>

#include "../methods/LocalSearchAlgorithm.hpp"
#include "../methods/MCTS.hpp"
#include "../methods/MemeticAlgorithm.hpp"
#include "../representation/Graph.hpp"
#include "../representation/Method.hpp"
#include "../representation/Parameters.hpp"

using json = nlohmann::json;

greedy_fct_ptr get_greedy_fct(const std::string &initialization);

std::shared_ptr<AdaptiveHelper> get_adaptive_helper(const json &data,
                                                    const int nb_operators,
                                                    const int nb_selected);

simulation_ptr get_simulation_fct(const std::string &simulation);

Crossover get_crossover(const json &data);

local_search_ptr get_local_search_fct(const std::string &local_search);

LocalSearch get_local_search(json data, int max_time, bool verbose,
                             long max_iterations);

selection_ptr get_selection_fct(const std::string &name);

insertion_ptr get_insertion_fct(const std::string &name);

ParamMA get_memetic(json data, int max_time, long max_iterations_);

ParamMCTS get_mcts(json data, int max_time, long max_iterations);

std::unique_ptr<Method> get_method(const std::string &json_content,
                                   int max_time, long max_iterations);
