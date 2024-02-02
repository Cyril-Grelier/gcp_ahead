#pragma once

#include "../representation/Solution.hpp"

struct ParamSelection {
    std::string name;
    int nb_selected;
};

typedef std::vector<std::pair<int, int>> (*selection_ptr)(const std::vector<Solution> &,
                                                          const ParamSelection &);

struct Selection {
    const selection_ptr function;
    const ParamSelection param;

    explicit Selection(const selection_ptr function_, const ParamSelection &param_);

    std::vector<std::pair<int, int>> run(const std::vector<Solution> &population) const;
};

std::vector<std::pair<int, int>> selection_random(const std::vector<Solution> &population,
                                                  const ParamSelection &param);

std::vector<std::pair<int, int>>
selection_random_closest(const std::vector<Solution> &population,
                         const ParamSelection &param);

std::vector<std::pair<int, int>>
selection_elitist(const std::vector<Solution> &population, const ParamSelection &param);

std::vector<std::pair<int, int>> selection_head(const std::vector<Solution> &population,
                                                const ParamSelection &param);
