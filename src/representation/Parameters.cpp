#include "Parameters.hpp"

#include <cstdio>
#include <fstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

#include "../utils/utils.hpp"

namespace parameters_search {
/** @brief The parameters of the search*/
std::unique_ptr<Parameters> parameters;
} // namespace parameters_search

Parameters::Parameters(const std::string &instance_,
                       const int nb_colors_,
                       const bool use_target_,
                       const int rand_seed_,
                       const std::chrono::high_resolution_clock::time_point &time_start_,
                       const int time_limit_,
                       const long max_iterations_,
                       const std::string &output_directory_,
                       const std::string &parameters_json)
    : nb_colors(nb_colors_),
      use_target(use_target_),
      rand_seed(rand_seed_),
      time_start(time_start_),
      time_limit(time_limit_),
      max_iterations(max_iterations_),
      time_stop(time_start_ + std::chrono::seconds(time_limit_)),
      output_directory(output_directory_) {
    // set output file if needed
    if (output_directory_ != "") {
        if (use_target) {
            output_file = fmt::format(
                "{}/{}_{}_{}.csv", output_directory_, instance_, rand_seed_, nb_colors);
        } else {
            output_file =
                fmt::format("{}/{}_{}.csv", output_directory_, instance_, rand_seed_);
        }
        std::FILE *file = std::fopen((output_file + ".running").c_str(), "w");
        if (!file) {
            fmt::print(stderr, "error while trying to access {}\n", output_file);
            exit(1);
        }
        output = file;
    } else {
        output = stdout;
    }

    fmt::print(output,
               "#date,"
               "problem,"
               "instance,"
               "nb_colors,"
               "use_target,"
               "rand_seed,"
               "objective,"
               "time_limit,"
               "max_iterations,"
               "parameters"
               "\n");
    fmt::print(output,
               "#{},{},{},{},{},{},{},{},{}\n",
               get_date_str(),
               "gcp",
               instance_,
               nb_colors,
               use_target,
               rand_seed_,
               time_limit,
               max_iterations,
               parameters_json);
}

void Parameters::end_search() const {
    fmt::print(output, "#{}\n", get_date_str());
    if (output != stdout) {
        std::fflush(output);
        std::fclose(output);
        if (std::rename((output_file + ".running").c_str(), output_file.c_str()) != 0) {
            fmt::print(
                stderr, "error while changing name of output file {}\n", output_file);
            exit(1);
        }
        fmt::print("{}\n", output_file);
    }
}

bool Parameters::time_limit_reached() const {
    return not(std::chrono::duration_cast<std::chrono::seconds>(
                   time_stop - std::chrono::high_resolution_clock::now())
                   .count() >= 0);
}

bool Parameters::time_limit_reached_sub_method(
    const std::chrono::high_resolution_clock::time_point &time) const {
    const auto now = std::chrono::high_resolution_clock::now();
    return not(std::chrono::duration_cast<std::chrono::seconds>(time - now).count() >=
               0) or
           not(std::chrono::duration_cast<std::chrono::seconds>(time_stop - now)
                   .count() >= 0);
}

int64_t Parameters::elapsed_time(
    const std::chrono::high_resolution_clock::time_point &time) const {
    return std::chrono::duration_cast<std::chrono::seconds>(time - time_start).count();
}
