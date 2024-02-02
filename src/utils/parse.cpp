#include "parse.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

using namespace graph_instance;
using namespace parameters_search;

greedy_fct_ptr get_greedy_fct(const std::string &initialization) {
  if (initialization == "random")
    return greedy_random;
  if (initialization == "constrained")
    return greedy_constrained;
  if (initialization == "deterministic")
    return greedy_deterministic;
  if (initialization == "deterministic_2")
    return greedy_deterministic_2;
  if (initialization == "adaptive")
    return greedy_adaptive;
  if (initialization == "DSatur")
    return greedy_DSatur;
  if (initialization == "RLF")
    return greedy_RLF;
  fmt::print(stderr, "Unknown initialization, please select : "
                     "random, constrained, deterministic, deterministic_2, "
                     "adaptive, DSatur, RLF\n");
  exit(1);
}

std::shared_ptr<AdaptiveHelper> get_adaptive_helper(const json &data,
                                                    const int nb_operators,
                                                    const int nb_selected) {
  std::string name = data["name"];
  int memory_size = data["memory_size"];
  int coeff_exploi_explo = data["coeff_exploi_explo"];

  ParamAdapt param{name, nb_operators, memory_size, nb_selected,
                   coeff_exploi_explo};

  if (name == "none")
    return std::make_shared<AdaptiveHelper_none>(param);
  if (name == "iterated")
    return std::make_shared<AdaptiveHelper_iterated>(param);
  if (name == "random")
    return std::make_shared<AdaptiveHelper_random>(param);
  if (name == "deleter")
    return std::make_shared<AdaptiveHelper_deleter>(param);
  if (name == "roulette_wheel")
    return std::make_shared<AdaptiveHelper_roulette_wheel>(param);
  if (name == "pursuit")
    return std::make_shared<AdaptiveHelper_pursuit>(param);
  if (name == "ucb")
    return std::make_shared<AdaptiveHelper_ucb>(param);
  if (name == "neural_net")
    return std::make_shared<AdaptiveHelper_neural_net>(param);
  if (name == "neural_net_cross")
    return std::make_shared<AdaptiveHelper_neural_net>(param);

  fmt::print(stderr,
             "Unknown adaptive : {}\n"
             "Please select : "
             "none, iterated, random, deleter, roulette_wheel, pursuit, ucb, "
             "neural_net, "
             "neural_net_cross\n",
             name);
  exit(1);
}

simulation_ptr get_simulation_fct(const std::string &simulation) {
  if (simulation == "no_ls") {
    return no_ls;
  }
  if (simulation == "always_ls") {
    return always_ls;
  }
  if (simulation == "fit") {
    return fit;
  }
  if (simulation == "depth") {
    return depth;
  }
  if (simulation == "level") {
    return level;
  }
  if (simulation == "depth_fit") {
    return depth_fit;
  }
  fmt::print(stderr,
             "Unknown simulation : {}\n"
             "Please select : "
             "no_ls, always_ls, fit, depth, level, depth_fit\n",
             simulation);
  exit(1);
}

Crossover get_crossover(const json &data) {
  int percentage_p1 = 50;
  int colors_p1 = 1;
  if (data.contains("percentage_p1")) {
    percentage_p1 = data["percentage_p1"];
  }
  if (data.contains("colors_p1")) {
    colors_p1 = data["colors_p1"];
  }
  ParamCrossover param =
      ParamCrossover{data["name"], data["pseudo"], percentage_p1, colors_p1};
  if (param.name == "no_crossover")
    return Crossover{no_crossover, param};
  if (param.name == "gpx")
    return Crossover{gpx, param};
  if (param.name == "partial_random_gpx")
    return Crossover{partial_random_gpx, param};
  if (param.name == "partial_best_gpx")
    return Crossover{partial_best_gpx, param};
  fmt::print(stderr,
             "Unknown crossover {}\nplease select : "
             "no_crossover, gpx, partial_random_gpx, partial_best_gpx\n",
             param.name);
  exit(1);
}

local_search_ptr get_local_search_fct(const std::string &local_search) {
  if (local_search == "none")
    return nullptr;
  if (local_search == "partial_col")
    return partial_col;
  if (local_search == "partial_ts")
    return partial_ts;
  if (local_search == "partial_col_optimized")
    return partial_col_optimized;
  if (local_search == "tabu_col")
    return tabu_col;
  if (local_search == "tabu_col_optimized")
    return tabu_col_optimized;
  if (local_search == "tabu_bucket")
    return tabu_bucket;
  fmt::print(
      stderr,
      "Unknown local_search, please select : "
      "none, partial_col, partial_col_optimized, partial_ts, tabu_bucket, "
      "tabu_col_optimized,tabu_col\n");
  exit(1);
}

LocalSearch get_local_search(json data, int max_time, bool verbose,
                             long max_iterations_) {
  const std::string name = data["name"];
  const std::string pseudo = data["pseudo"];
  if (name == "tabu_bucket" and qgraph == nullptr) {
    init_UBQP();
  }
  double alpha = 0;
  int random_min = 0;
  int random_max = 0;
  if (data.contains("tabu_iter")) {
    alpha = data["tabu_iter"]["alpha"];
    random_min = data["tabu_iter"]["random"]["min"];
    random_max = data["tabu_iter"]["random"]["max"];
  }
  long max_iterations = max_iterations_;
  if (data.contains("time")) {
    if (data["time"].contains("relative")) {
      double relative = data["time"]["relative"];
      max_time = static_cast<int>(graph->nb_vertices * relative);
    } else if (data["time"].contains("fixed")) {
      max_time = data["time"]["fixed"];
    } else if (data["time"].contains("iterations")) {
      max_iterations = data["time"]["iterations"];
    } else {
      fmt::print(stderr,
                 "time must be choose between fixed, relative or iterations {}",
                 data);
      exit(1);
    }
  }
  return LocalSearch(get_local_search_fct(name),
                     ParamLS{name, pseudo, alpha, random_min, random_max,
                             max_time, max_iterations, verbose});
}

selection_ptr get_selection_fct(const std::string &name) {
  if (name == "selection_random")
    return selection_random;
  if (name == "selection_random_closest")
    return selection_random_closest;
  if (name == "selection_head")
    return selection_head;

  fmt::print(stderr,
             "Unknown selection ({}), please select : "
             "selection_random, selection_random_closest, selection_head (only "
             "in head "
             "configuration)\n",
             name);
  exit(1);
}

insertion_ptr get_insertion_fct(const std::string &name) {
  // if (name == "insertion_best")
  //     return insertion_best;
  if (name == "insertion_distance")
    return insertion_distance;
  if (name == "insertion_head") {
    return nullptr;
  }

  fmt::print(
      stderr,
      "Unknown insertion ({}), please select : "
      "insertion_distance, insertion_head (only in head configuration)\n",
      name);
  exit(1);
}

ParamMA get_memetic(json data, int max_time, long max_iterations_) {
  const std::string name = data["name"];
  int population_size = data["population_size"];
  int nb_selected = data["nb_selected"];
  std::vector<LocalSearch> local_search;
  for (auto json_ls : data["local_search"]) {
    local_search.emplace_back(get_local_search(
        json_ls, max_time, false, std::numeric_limits<long>::max()));
  }
  std::vector<Crossover> crossover;
  for (auto json_crossover : data["crossover"]) {
    crossover.emplace_back(get_crossover(json_crossover));
  }
  int nb_operators = static_cast<int>(local_search.size() * crossover.size());
  std::vector<std::tuple<int, int>> pairs_x_ls;
  std::vector<std::string> x_ls_names;
  for (int i = 0; i < static_cast<int>(crossover.size()); i++) {
    for (int j = 0; j < static_cast<int>(local_search.size()); j++) {
      pairs_x_ls.emplace_back(i, j);
      x_ls_names.emplace_back(fmt::format("{}_{}", crossover[i].param.pseudo,
                                          local_search[j].param.pseudo));
    }
  }

  long max_iterations = max_iterations_;

  if (data.contains("time")) {
    if (data["time"].contains("relative")) {
      double relative = data["time"]["relative"];
      max_time = static_cast<int>(graph->nb_vertices * relative);
    } else if (data["time"].contains("fixed")) {
      max_time = data["time"]["fixed"];
    } else if (data["time"].contains("iterations")) {
      max_iterations = data["time"]["iterations"];
    } else {
      fmt::print(stderr,
                 "time must be choose between fixed, relative or iterations {}",
                 data);
      exit(1);
    }
  }
  Selection selection{get_selection_fct(data["selection"]["name"]),
                      {data["selection"]["name"], nb_selected}};
  Insertion insertion{get_insertion_fct(data["insertion"]["name"]),
                      {data["insertion"]["name"], population_size}};
  if (data["adaptive"]["name"] == "neural_net") {
    nb_operators = static_cast<int>(local_search.size());
  }
  long elites = 1;
  if (data.contains("elites")) {
    elites = data["elites"];
  }
  return ParamMA{
      name,
      population_size,
      nb_selected,
      max_time,
      max_iterations,
      selection,
      crossover,
      local_search,
      insertion,
      pairs_x_ls,
      x_ls_names,
      get_adaptive_helper(data["adaptive"], nb_operators, nb_selected),
      elites};
}

ParamMCTS get_mcts(json data, int max_time, long max_iterations_) {

  const std::string name;
  std::vector<LocalSearch> local_search;
  std::vector<std::string> ls_names;
  std::shared_ptr<AdaptiveHelper> adaptive_helper;

  for (auto json_ls : data["local_search"]) {
    local_search.emplace_back(get_local_search(
        json_ls, max_time, false, std::numeric_limits<long>::max()));
  }
  for (int j = 0; j < static_cast<int>(local_search.size()); j++) {
    ls_names.emplace_back(local_search[j].param.pseudo);
  }
  long max_iterations = max_iterations_;

  if (data.contains("time")) {
    if (data["time"].contains("relative")) {
      double relative = data["time"]["relative"];
      max_time = static_cast<int>(graph->nb_vertices * relative);
    } else if (data["time"].contains("fixed")) {
      max_time = data["time"]["fixed"];
    } else if (data["time"].contains("iterations")) {
      max_iterations = data["time"]["iterations"];
    } else {
      fmt::print(stderr,
                 "time must be choose between fixed, relative or iterations {}",
                 data);
      exit(1);
    }
  }
  return ParamMCTS{data["name"],
                   max_time,
                   max_iterations,
                   local_search,
                   ls_names,
                   get_adaptive_helper(data["adaptive"],
                                       static_cast<int>(local_search.size()),
                                       1),
                   get_simulation_fct(data["simulation"]["name"])};
}

std::unique_ptr<Method> get_method(const std::string &json_content,
                                   int max_time, long max_iterations) {
  json data = json::parse(json_content);
  // std::cout << data << std::endl;
  if (not data.contains("method")) {
    fmt::print(stderr,
               "json file without method see README.md or files examples in "
               "parameter folder");
  } else if (data["method"] == "greedy") {
    return std::make_unique<GreedyAlgorithm>(
        get_greedy_fct(data["initialization"]));
  } else if (data["method"] == "local_search") {
    return std::make_unique<LocalSearchAlgorithm>(
        get_greedy_fct(data["initialization"]),
        get_local_search(data, max_time, true, max_iterations));
  } else if (data["method"] == "memetic") {
    return std::make_unique<MemeticAlgorithm>(
        get_greedy_fct(data["initialization"]),
        get_memetic(data, max_time, max_iterations));
  } else if (data["method"] == "mcts") {
    return std::make_unique<MCTS>(get_greedy_fct(data["initialization"]),
                                  get_mcts(data, max_time, max_iterations));
  } else {
    fmt::print(stderr, "unkown method : {}", data["method"]);
    exit(1);
  }

  exit(1);
}
