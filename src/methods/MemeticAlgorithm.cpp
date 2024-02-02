#include "MemeticAlgorithm.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

#include "../representation/Graph.hpp"
#include "../representation/Parameters.hpp"
#include "../utils/random_generator.hpp"
#include "../utils/utils.hpp"
#include "insertion.hpp"
#include "selection.hpp"

using namespace graph_instance;
using namespace parameters_search;

MemeticAlgorithm::MemeticAlgorithm(greedy_fct_ptr greedy_function_, const ParamMA &param_)
    : _best_solution(),
      _param(param_),
      _greedy_function(greedy_function_),
      _population(param_.population_size),
      _t_best(std::chrono::high_resolution_clock::now()) {
    if (parameters->output_file != "") {
        output_file_tbt = fmt::format("{}/tbt/{}_{}_{}.csv",
                                      parameters->output_directory,
                                      graph->name,
                                      parameters->rand_seed,
                                      parameters->nb_colors);

        std::FILE *file_tbt = std::fopen((output_file_tbt + ".running").c_str(), "w");
        if (!file_tbt) {
            throw std::runtime_error(
                fmt::format("error while trying to access {}\n", output_file_tbt));
        }
        output_tbt = file_tbt;
    } else {
        output_tbt = stdout;
    }
    // _selected.reserve(_param.nb_selected);
    // _children.reserve(_param.nb_selected);

    fmt::print(parameters->output, "{}", header_csv());

    // operators_str.pop_back();
    fmt::print(output_tbt, "#operators\n");
    fmt::print(output_tbt, "#{}\n", fmt::join(_param.x_ls_names, ":"));

    fmt::print(output_tbt,
               "time,turn,distance_parents,parents,proba,selected,"
               "fitness_pre_crossover,fitness_pre_mutation,"
               "fitness_post_mutation,min_fitness,mean_fitness,max_fitness,"
               "age,min_distance,mean_distance,max_distance\n");
}

MemeticAlgorithm::~MemeticAlgorithm() {
    if (output_tbt != stdout) {
        std::fflush(output_tbt);
        std::fclose(output_tbt);
        if (std::rename((output_file_tbt + ".running").c_str(),
                        output_file_tbt.c_str()) != 0) {
            fmt::print(
                stderr, "error while changing name of output file {}\n", output_file_tbt);
            exit(1);
        }
    }
}

bool MemeticAlgorithm::stop_condition() const {
    return (_turn < _param.max_iterations) and (not parameters->time_limit_reached()) and
           not(_best_solution.penalty() == 0 and _best_solution.nb_uncolored() == 0)
        // to check if insertion added enough individuals
        // (doesn't add individual when distance of 0 to others)
        // and static_cast<int>(_population.size()) >= _param.nb_selected
        ;
}

void MemeticAlgorithm::run() {
    // init population
#pragma omp parallel for
    for (int i = 0; i < _param.population_size; i++) {
        _greedy_function(_population[i]);
        _population[i] = _population[i].reduce_nb_colors_illegal(parameters->nb_colors);
        assert(_population[i].nb_colors() == parameters->nb_colors or
               _population[i].penalty() == 0);
    }

    std::stable_sort(_population.begin(),
                     _population.end(),
                     [](const Solution &indiv1, const Solution &indiv2) {
                         return indiv1.penalty() < indiv2.penalty();
                     });

    // compute the distances
    for (int i = 0; i < _param.population_size; i++) {
        for (int j = i + 1; j < _param.population_size; j++) {
            const int dist = distance_accurate(_population[i], _population[j]);
            _population[i].distances[_population[j].id] = dist;
            _population[j].distances[_population[i].id] = dist;
        }
    }
    std::vector<Solution> elites(_population);
    int current_elite = 0;
    std::uniform_int_distribution<int> distribution_elite(0, 1);
    int threshold = static_cast<int>(graph->nb_vertices * 0.99);

    update_best_score();
    std::string fit_str = "";
    std::string selected_str = "";
    while (stop_condition()) {
        // if the 2 individual for HEAD are equal
        if (_param.insertion.param.name == "insertion_head" and
            _population[0].distances[_population[1].id] == 0) {
            // restart the population and elites
            _population = std::vector<Solution>(_param.population_size);
            elites = std::vector<Solution>(_param.population_size);
            for (int i = 0; i < _param.population_size; ++i) {
                _greedy_function(_population[i]);
                _population[i] =
                    _population[i].reduce_nb_colors_illegal(parameters->nb_colors);
                assert(_population[i].nb_colors() == parameters->nb_colors);
            }
            std::stable_sort(_population.begin(),
                             _population.end(),
                             [](const Solution &indiv1, const Solution &indiv2) {
                                 return indiv1.penalty() < indiv2.penalty();
                             });
            for (int i = 0; i < _param.population_size; i++) {
                for (int j = i + 1; j < _param.population_size; j++) {
                    const int dist = distance_accurate(_population[i], _population[j]);
                    _population[i].distances[_population[j].id] = dist;
                    _population[j].distances[_population[i].id] = dist;
                }
            }
            _greedy_function(elites[0]);
            elites[0] = elites[0].reduce_nb_colors_illegal(parameters->nb_colors);
            _greedy_function(elites[1]);
            elites[1] = elites[1].reduce_nb_colors_illegal(parameters->nb_colors);
        }
        // Selection of parents
        _selected = _param.selection.run(_population);

        std::string selected_indiv_str = "";
        std::string selected_distance_str = "";
        for (const auto &[p1, p2] : _selected) {
            selected_indiv_str += fmt::format("{}:{}:", p1, p2);
            selected_distance_str +=
                fmt::format("{}:", _population[p1].distances.at(_population[p2].id));
        }

        // remove last ":"
        selected_indiv_str.pop_back();
        selected_distance_str.pop_back();

        // Crossover and local search
        if (_param.adaptive_helper->param.name == "neural_net") {
            std::tie(fit_str, selected_str) = crossover_and_mutation_neural_network();
        } else {
            fit_str = crossover_and_mutation();
            selected_str = _param.adaptive_helper->get_selected_str();
        }

        // Insertion
        if (_param.insertion.param.name == "insertion_head") {
            insertion_head();
        } else {
            insertion();
        }

        if (_population[0].penalty() <= elites[current_elite].penalty()) {
            elites[current_elite] = _population[0];
        }

        update_best_score();

        turn_by_turn_line(
            selected_indiv_str, selected_distance_str, fit_str, selected_str);

        ++_turn;
        for (auto &indiv : _population) {
            ++indiv.age;
        }

        // reintroduce elites in the population (for a HEAD configuration)
        // elites to -1 or 1 mean no elites
        // other values mean the frequency of elites
        if ((_turn % _param.elites) == 1) { // elites = 10 by default in HEAD
            current_elite = (current_elite + 1) % 2;
            int indiv_to_replace = distribution_elite(rd::generator);

            auto dist =
                distance_accurate(_population[indiv_to_replace], elites[current_elite]);
            if (dist > threshold) {
                indiv_to_replace = (indiv_to_replace + 1) % 2;
                dist = distance_accurate(_population[indiv_to_replace],
                                         elites[current_elite]);
            }
            _population[indiv_to_replace] = elites[current_elite];
            _population[indiv_to_replace].distances.clear();
            _population[(indiv_to_replace + 1) % 2].distances.clear();
            _population[0].distances[_population[1].id] = dist;
            _population[1].distances[_population[0].id] = dist;
        }
    }

    fmt::print(parameters->output, "{}", line_csv());
}

std::string MemeticAlgorithm::crossover_and_mutation() {
    std::string fit_str = "";
    // add score to the output
    for (const auto &parents : _selected) {
        fit_str += fmt::format("{}:", _population[parents.first].penalty());
    }
    fit_str.pop_back();
    fit_str += ",";

    std::vector<int> pair_operators(_param.nb_selected, -1);

    if (auto *cast =
            dynamic_cast<AdaptiveHelper_neural_net *>(_param.adaptive_helper.get())) {
        for (int i = 0; i < _param.nb_selected; ++i) {
            pair_operators[i] = cast->get_operator(i, _population[_selected[i].first]);
        }
    } else {
        for (int i = 0; i < _param.nb_selected; ++i) {
            pair_operators[i] = _param.adaptive_helper->get_operator();
        }
    }

    _children = std::vector<Solution>(_param.nb_selected);

    // Crossover
#pragma omp parallel for
    for (int i = 0; i < _param.nb_selected; ++i) {
        auto &[cross, ls] = _param.pairs_x_ls[pair_operators[i]];
        _param.crossover[cross].run(_population[_selected[i].first],
                                    _population[_selected[i].second],
                                    _children[i]);
        assert(_children[i].nb_colors() == parameters->nb_colors);
    }

    for (const auto &child : _children) {
        fit_str += fmt::format("{}:", child.penalty());
    }
    fit_str.pop_back();
    fit_str += ",";

    // Local Search
#pragma omp parallel for
    for (int i = 0; i < _param.nb_selected; i++) {
        auto &[cross, ls] = _param.pairs_x_ls[pair_operators[i]];
        assert(_children[i].nb_colors() == parameters->nb_colors);
        _param.local_search[ls].run(_children[i]);
        assert(_children[i].check_solution());
        assert(_children[i].nb_colors() == parameters->nb_colors);
    }

    for (int i = 0; i < _param.nb_selected; i++) {
        _param.adaptive_helper->update_obtained_solution(
            i, pair_operators[i], _children[i].penalty());
    }

    _param.adaptive_helper->update_helper();

    for (const auto &child : _children) {
        fit_str += fmt::format("{}:", child.penalty());
    }
    fit_str.pop_back();

    return fit_str;
}

std::tuple<std::string, std::string>
MemeticAlgorithm::crossover_and_mutation_neural_network() {
    auto *casted_adaptive =
        dynamic_cast<AdaptiveHelper_neural_net *>(_param.adaptive_helper.get());
    if (not casted_adaptive) {
        fmt::print(stderr, "Error while dynamic cast of AdaptiveHelper_neural_net\n");
        exit(1);
    }

    std::string fit_str = "";
    for (const auto &parents : _selected) {
        fit_str += fmt::format("{}:", _population[parents.first].penalty());
    }
    fit_str.pop_back();
    fit_str += ",";

    // compute all possible crossovers from 2 parents
    // const int nb_operator = static_cast<int>(_crossover_cb.size());

    _children = std::vector<Solution>(_param.nb_selected);

    std::vector<int> selected_crossover(_param.nb_selected, -1);

    std::vector<std::vector<Solution>> childrens(_param.nb_selected);
    for (int i = 0; i < _param.nb_selected; ++i) {
        childrens[i] = std::vector<Solution>(_param.crossover.size());
    }

#pragma omp parallel for
    for (int i = 0; i < _param.nb_selected; ++i) {
        auto children = childrens[i];
        for (size_t o = 0; o < _param.crossover.size(); ++o) {
            _param.crossover[o].run(_population[_selected[i].first],
                                    _population[_selected[i].second],
                                    children[o]);
        }
        const int best_child = casted_adaptive->select_best(children);
        selected_crossover[i] = best_child;
        _children[i] = children[best_child];
    }

    for (const auto &child : _children) {
        fit_str += fmt::format("{}:", child.penalty());
    }
    fit_str.pop_back();
    fit_str += ",";

    // select the local search
    std::vector<int> selected_local_search(_param.nb_selected, -1);
    for (int i = 0; i < _param.nb_selected; ++i) {
        selected_local_search[i] = casted_adaptive->get_operator(i, _children[i]);
    }

    // Local search

#pragma omp parallel for
    for (int i = 0; i < _param.nb_selected; i++) {
        _param.local_search[selected_local_search[i]].run(_children[i]);
    }

    for (int i = 0; i < _param.nb_selected; i++) {
        _param.adaptive_helper->update_obtained_solution(
            i, selected_local_search[i], _children[i].penalty());
    }

    _param.adaptive_helper->update_helper();

    for (const auto &child : _children) {
        fit_str += fmt::format("{}:", child.penalty());
    }
    fit_str.pop_back();

    std::string selected_str = "";
    for (int i = 0; i < _param.nb_selected; ++i) {
        selected_str += fmt::format("{}:",
                                    selected_crossover[i] *
                                            static_cast<int>(_param.local_search.size()) +
                                        selected_local_search[i]);
    }
    selected_str.pop_back();

    return std::make_pair(fit_str, selected_str);
}

void MemeticAlgorithm::insertion() {
    while (not _children.empty()) {
        auto child = _children.back();
        _children.pop_back();
        insert(child);
    }
}

void MemeticAlgorithm::insertion_head() {
    _population = _children;
    _children.clear();

    const int dist = distance_accurate(_population[0], _population[1]);
    _population[0].distances[_population[1].id] = dist;
    _population[1].distances[_population[0].id] = dist;

    // sort the population by penalty
    std::stable_sort(_population.begin(),
                     _population.end(),
                     [](const Solution &s1, const Solution &s2) {
                         return s1.penalty() < s2.penalty();
                     });
}

void MemeticAlgorithm::insert(Solution &child) {
    // Compute the distances to the child
    // std::vector<int> distances_to_child(_param.population_size, 0);
    for (size_t i = 0; i < _population.size(); ++i) {
        const int dist = distance_accurate(child, _population[i]);
        child.distances[_population[i].id] = dist;
        _population[i].distances[child.id] = dist;
        // distances_to_child[i] = distance_accurate(_population[i], child);
    }

    int to_remove = _param.insertion.run(_population, child);

    assert(to_remove >= -1);
    assert(to_remove < static_cast<int>(_population.size()));

    if (to_remove == -1) {
        // the worst solution is the child so it should not be added
        // but the child as 10% chance to be accepted anyway
        std::uniform_int_distribution<int> distribution(1, 100);
        if (distribution(rd::generator) < 10) {
            // the child is accepted
            // so the worst solution is removed
            to_remove = static_cast<int>(_population.size()) - 1;
        }
    }

    // if the child is not inserted, remove it from the distances
    if (to_remove == -1) {
        for (size_t i = 0; i < _population.size(); ++i) {
            _population[i].distances.erase(child.id);
        }
        return;
    }

    // remove the deleted solution from the population
    // and from the distances of each solution of the population
    auto id_to_remove = _population[to_remove].id;
    for (size_t i = 0; i < _population.size(); ++i) {
        _population[i].distances.erase(id_to_remove);
    }
    child.distances.erase(id_to_remove);
    // remove the to_remove index from the population
    _population.erase(_population.begin() + to_remove);

    _population.push_back(child);

    // sort the population by penalty
    std::stable_sort(_population.begin(),
                     _population.end(),
                     [](const Solution &s1, const Solution &s2) {
                         return s1.penalty() < s2.penalty();
                     });
}

[[nodiscard]] const std::string MemeticAlgorithm::header_csv() const {
    return fmt::format("turn,time,{}\n", Solution::header_csv);
}

[[nodiscard]] const std::string MemeticAlgorithm::line_csv() const {
    return fmt::format(
        "{},{},{}\n", _turn, parameters->elapsed_time(_t_best), _best_solution.format());
}

void MemeticAlgorithm::update_best_score() {
    if (std::max(_best_solution.penalty(), _best_solution.nb_uncolored()) >
        _population[0].penalty()) {
        _t_best = std::chrono::high_resolution_clock::now();
        _best_solution = _population[0];
        fmt::print(parameters->output, "{}", line_csv());
    }
}

void MemeticAlgorithm::turn_by_turn_line(const std::string &selected_indiv_str,
                                         const std::string &selected_distance_str,
                                         const std::string &fit_str,
                                         const std::string &selected_str) {
    double mean_fit = 0;
    double mean_dist = 0;
    double min_dist = graph->nb_vertices;
    double max_dist = 0;
    int nb_distances = 0;
    for (size_t indiv = 0; indiv < _population.size(); ++indiv) {
        mean_fit += _population[indiv].penalty();
        for (size_t indiv2 = 0; indiv2 < _population.size(); ++indiv2) {
            if (indiv == indiv2) {
                continue;
            }
            const int distance_ = _population[indiv].distances.at(_population[indiv2].id);
            mean_dist += distance_;
            ++nb_distances;
            if (distance_ < min_dist) {
                min_dist = distance_;
            }
            if (distance_ > max_dist) {
                max_dist = distance_;
            }
        }
    }
    mean_dist /= nb_distances;
    mean_fit /= static_cast<double>(_population.size());

    std::string ages{};
    for (const auto &indiv : _population) {
        ages += std::to_string(indiv.age) + ":";
    }
    // delete the last ":"
    ages.pop_back();

    fmt::print(output_tbt,
               "{},{},{},{},{},{},{},{},{:.1f},{},{},{},{:.1f},{}\n",
               parameters->elapsed_time(std::chrono::high_resolution_clock::now()),
               _turn,
               selected_distance_str,
               selected_indiv_str,
               _param.adaptive_helper->to_str_proba(),
               selected_str,
               fit_str,
               _population[0].penalty(),
               mean_fit,
               _population.back().penalty(),
               ages,
               min_dist,
               mean_dist,
               max_dist);
    _param.adaptive_helper->increment_turn();
}
