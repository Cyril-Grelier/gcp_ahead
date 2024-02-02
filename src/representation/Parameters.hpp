#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Representation of parameters
 *
 */
struct Parameters {

    const int nb_colors;
    const bool use_target;
    const int rand_seed;
    const std::chrono::high_resolution_clock::time_point time_start;
    const int time_limit;
    const long max_iterations;
    /** @brief time limit for the algorithm, can be set to now to stop*/
    std::chrono::high_resolution_clock::time_point time_stop;
    const std::string output_directory;

    /** @brief Output file name if not on console*/
    std::string output_file;
    /** @brief Output, stdout default*/
    std::FILE *output = nullptr;

    /**
     * @brief Construct Parameters
     */
    explicit Parameters(const std::string &instance_,
                        const int nb_colors_,
                        const bool use_target_,
                        const int rand_seed_,
                        const std::chrono::high_resolution_clock::time_point &time_start_,
                        const int time_limit_,
                        const long max_iterations_,
                        const std::string &output_directory_,
                        const std::string &parameters_json);

    /**
     * @brief Close output file if needed
     *
     */
    void end_search() const;

    /**
     * @brief Return true if the time limit is reached
     *
     * @return true Time limit is reached
     * @return false The search continue
     */
    bool time_limit_reached() const;

    /**
     * @brief Return true if the time limit is reached according to the given time
     *
     * @return true Time limit is reached
     * @return false The search continue
     */
    bool time_limit_reached_sub_method(
        const std::chrono::high_resolution_clock::time_point &time) const;

    /**
     * @brief Returns the number of seconds between the given time and the start of
     * the search
     *
     * @param time given time (std::chrono::high_resolution_clock::now())
     * @return int64_t elapsed time in seconds
     */
    int64_t
    elapsed_time(const std::chrono::high_resolution_clock::time_point &time) const;
};

namespace parameters_search {

/** @brief The parameters of the search*/
extern std::unique_ptr<Parameters> parameters;
} // namespace parameters_search
