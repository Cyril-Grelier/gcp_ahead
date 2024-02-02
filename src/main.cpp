#include <csignal>
#include <fstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "cxxopts.hpp"
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

#include "utils/parse.hpp"
#include "utils/random_generator.hpp"

using namespace graph_instance;
using namespace parameters_search;

/**
 * @brief Signal handler to let the algorithm to finish its last turn
 */
void signal_handler(int signum);

/**
 * @brief parse the argument for the search and return the method to run
 */
std::unique_ptr<Method> parse(int argc, const char **argv);

int main(int argc, const char *argv[]) {
    // see parse function for default parameters

    // Get the method
    auto method(parse(argc, argv));

    // Set the signal handler to stop the search
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Start the search
    method->run();
    parameters->end_search();
}

void signal_handler(int signum) {
    fmt::print(stderr, "\nInterrupt signal ({}) received.\n", signum);
    parameters->time_stop = std::chrono::high_resolution_clock::now();
}

std::unique_ptr<Method> parse(int argc, const char **argv) {
    const auto time_start = std::chrono::high_resolution_clock::now();

    // analyse command line options
    try {
        // init cxxopts
        cxxopts::Options options(argv[0], "Program to launch : mcts, local search\n");

        options.positional_help("[optional args]").show_positional_help();

        options.allow_unrecognised_options().add_options()("h,help", "Print usage");

        /****************************************************************************
         *
         *                      Set defaults values down here
         *
         ***************************************************************************/

        options.allow_unrecognised_options().add_options()(
            "i,instance",
            "name of the instance (located in instance/wvcp_reduced/)",
            cxxopts::value<std::string>()->default_value(
                //
                // "0_test"
                // "0_test_1"
                // "r1000.1c"
                // "DSJC1000.9"
                // "DSJR500.1"
                "r1000.5" // 234
                // "queen10_10" // 11
                // "queen11_11" // 11
                // "queen12_12" // 12
                // "le450_25c" // 25
                // "DSJC1000.1" // k=20
                // "DSJC125.1" // k=5 *
                // "DSJC125.9" // k=44 *
                // "DSJC500.1" // k=12 -
                // "wap06a" // k=39
                // "DSJC500.5" // k=47 -
                // "inithx.i.1" // 31
                // "DSJC1000.1" // k=20 -
                // "DSJC500.9" // k=126 -
                // "C2000.5" // 146
                //
                ));

        options.allow_unrecognised_options().add_options()(
            "k,nb_colors",
            "number of colors",
            cxxopts::value<int>()->default_value(
                //
                "234"
                //
                ));

        options.allow_unrecognised_options().add_options()(
            "T,use_target",
            "true : for local search, start the search from nb_colors, "
            "false : start from the number of color after greedy and reduce "
            "color by color when a legal solution is found",
            cxxopts::value<std::string>()->default_value(
                //
                "true"
                // "false"
                //
                ));

        options.allow_unrecognised_options().add_options()(
            "r,rand_seed",
            "random seed",
            cxxopts::value<int>()->default_value(
                //
                // "0"
                // std::to_string(time(nullptr))
                "9"
                // "18"
                //
                ));

        const std::string time_limit_default = "3600";
        options.allow_unrecognised_options().add_options()(
            "t,time_limit",
            "maximum execution time in seconds",
            cxxopts::value<int>()->default_value(time_limit_default));

        options.allow_unrecognised_options().add_options()(
            "n,nb_iterations",
            "maximum iterations (can be overload by iterations in parameters file)",
            cxxopts::value<long>()->default_value(
                //
                std::to_string(std::numeric_limits<long>::max())
                //
                ));

        options.allow_unrecognised_options().add_options()(
            "p,parameters",
            "see parameters folder",
            cxxopts::value<std::string>()->default_value(
                //
                // "../parameters/local_search/partial_col.json"
                "../parameters/local_search/tabu_col_optimized.json"
                // "../parameters/local_search/partial_col_optimized.json"
                // "../parameters/local_search/partial_ts.json"
                // "../parameters/memetic/macol_bgpx_pco.json"
                // "../parameters/memetic/macol_gpx_pco.json"
                // "../parameters/memetic/macol_gpx_pts.json"
                // "../parameters/memetic/macol_gpx_tco.json"
                // "../parameters/memetic/macol_random.json"
                // "../parameters/memetic/macol_roulette.json"
                // "../parameters/memetic/macol_deleter.json"
                // "../parameters/memetic/macol_pursuit.json"
                // "../parameters/memetic/macol_ucb.json"
                // "../parameters/memetic/macol_neural_net.json"
                // "../parameters/memetic/macol.json"
                // "../parameters/memetic/head.json"
                // "../parameters/memetic/head_ucb_ls_alpha.json"
                // "../parameters/memetic/head_pc_ls_0.001.json"
                // "../parameters/memetic/head_neural_net.json"
                // "../parameters/memetic/macol_neural_net.json"
                // "../parameters/mcts/mcts_ucb.json"
                // "../parameters/mcts/mcts_greedy_random.json"
                // "../parameters/mcts/mcts_greedy_constrained.json"
                // "../parameters/mcts/mcts_greedy_deterministic.json"
                // "../parameters/mcts/mcts_greedy_adaptive.json"
                // "../parameters/mcts/mcts_greedy_dsatur.json"
                // "../parameters/mcts/mcts_greedy_rlf.json"
                // "../parameters/mcts/mcts_tco.json"
                // "../parameters/mcts/mcts_pts.json"
                // "../parameters/mcts/mcts_pco.json"
                //
                ));

        options.allow_unrecognised_options().add_options()(
            "o,output_directory",
            "output file, let empty if output to stdout, else directory, file name "
            "will "
            "be [instance name]_[rand seed]_[nb_colors].csv (.running if not "
            "finished) "
            "add a tbt repertory for the turn by turn informations",
            cxxopts::value<std::string>()->default_value(""));

        /****************************************************************************
         *
         *                      Set defaults values up here
         *
         ***************************************************************************/

        const auto result = options.parse(argc, const_cast<char **&>(argv));

        // help message
        if (result.count("help")) {
            // load instance names
            std::ifstream i_file("../instances/instance_list_gcp.txt");
            if (!i_file) {
                fmt::print(stderr,
                           "Unable to find : ../instances/instance_list_gcp.txt\n"
                           "Check if you imported the submodule instance, commands :\n"
                           "\tgit submodule init\n"
                           "\tgit submodule update\n");
                exit(1);
            }
            std::string tmp;
            std::vector<std::string> instance_names;
            while (!i_file.eof()) {
                i_file >> tmp;
                instance_names.push_back(tmp);
            }
            i_file.close();
            // print help
            fmt::print(stdout,
                       "{}\nInstances :\n{}\n",
                       options.help(),
                       fmt::join(instance_names, " "));
            exit(0);
        }

        const std::string instance = result["instance"].as<std::string>();

        // load graph
        load_graph(instance);

        int nb_colors = result["nb_colors"].as<int>();
        const bool use_target = result["use_target"].as<std::string>() == "true";

        // if (nb_colors == -1) {
        //     std::ifstream i_file("../instances/best_scores_gcp.txt");
        //     if (!i_file) {
        //         fmt::print(stderr,
        //                    "Unable to find : ../instances/best_scores_gcp.txt\n"
        //                    "Check if you imported the submodule instance,
        //                    commands :\n"
        //                    "\tgit submodule init\n"
        //                    "\tgit submodule update\n");
        //         exit(1);
        //     }
        //     std::string inst;
        //     int val;
        //     char optimality;
        //     std::vector<std::string> instance_names;
        //     int i = 0;
        //     while (!i_file.eof()) {
        //         i_file >> inst >> val >> optimality;
        //         if (inst == instance) {
        //             if (optimality == '*') {
        //                 nb_colors = val;
        //             } else if (use_target) {
        //                 fmt::print(stderr,
        //                            "error: the number of colors must be given if
        //                            the " "optimal solution is not known and the
        //                            search use it " "as target\n");
        //                 exit(1);
        //             }
        //             break;
        //         }
        //         ++i;
        //     }
        //     i_file.close();
        // }
        if (nb_colors == -1) {
            nb_colors = graph->nb_vertices;
        }

        const int time_limit = result["time_limit"].as<int>();
        const long max_iterations = result["nb_iterations"].as<long>();

        const int rand_seed = result["rand_seed"].as<int>();
        rd::generator.seed(rand_seed);

        const std::string output_directory = result["output_directory"].as<std::string>();
        const std::string json_file = result["parameters"].as<std::string>();

        std::ifstream j_file(json_file);
        if (!j_file) {
            fmt::print(stderr, "Unable to find : {}\n", json_file);
            exit(1);
        }
        std::stringstream buffer;
        buffer << j_file.rdbuf();
        std::string parameters_json = buffer.str();
        j_file.close();

        // remove \n and spaces from the file for printing in csv file
        parameters_json.erase(
            std::remove(parameters_json.begin(), parameters_json.end(), '\n'),
            parameters_json.end());

        parameters_json.erase(
            std::remove(parameters_json.begin(), parameters_json.end(), ' '),
            parameters_json.end());

        // init parameters
        parameters = std::make_unique<Parameters>(instance,
                                                  nb_colors,
                                                  use_target,
                                                  rand_seed,
                                                  time_start,
                                                  time_limit,
                                                  max_iterations,
                                                  output_directory,
                                                  parameters_json);
        return get_method(parameters_json, time_limit, max_iterations);

    } catch (const cxxopts::OptionException &e) {
        fmt::print(stderr, "error parsing options: {} \n", e.what());
        exit(1);
    }
}
