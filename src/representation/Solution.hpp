#pragma once

#include <map>
#include <string>
#include <vector>

/**
 * @brief Represent the action of moving a vertex to a color
 *
 */
struct Coloration {
    /** @brief vertex to color*/
    int vertex;
    /** @brief color to use*/
    int color;
};

/**
 * @brief A solution is represented by the color of each vertex
 * -1 if the vertex is uncolored
 */
class Solution {

    /** @brief counter of solutions */
    static ulong counter;

  public:
    /** @brief ID of the solution.
     * Warning! local search can create new solutions (when use_target is false)
     * be careful if launched in parallel and distance are useful*/
    ulong id;

    static int best_penalty;
    static int best_nb_colors;

  private:
    /** @brief for each vertex, its color */
    std::vector<int> _colors;
    /** @brief number of colors currently used in the solution */
    int _nb_colors;
    /** @brief set of uncolored vertices (sorted vector) */
    std::vector<int> _uncolored;
    /** @brief number of constraint not respected */
    int _penalty;
    /** @brief for each color, the number of vertices colored with it */
    std::vector<int> _color_size{};
    /** @brief for each color, for each vertex, its number of conflicts */
    std::vector<std::vector<int>> _conflicts{};
    /** @brief set of each vertex in conflicts (sorted vector) */
    std::vector<int> _conflicting_vertices{};

    // /** @brief for each vertex, the number of available colors */
    // std::vector<int> _nb_free_colors{};

    /** @brief for each color, for each vertex
     * the cost on the penalty if its moved there
     * for tabu col optimized */
    std::vector<std::vector<int>> _deltas{};
    /** @brief for each vertex, best transition cost
     * for tabu col optimized */
    std::vector<int> _best_delta{};
    /** @brief for each vertex, set of best colors (vector of sorted vector)
     * for tabu col optimized */
    std::vector<std::vector<int>> _best_improve_colors{};
    /** @brief for each vertex, set of possible colors (vector of sorted vector)
     * for partial col optimized */
    std::vector<std::vector<int>> _possible_colors{};

    /** @brief Next vertex to color in the MCTS tree*/
    int _first_free_vertex{0};

  public:
    /** @brief Age of the solution for the memetic algorithm */
    int age = 0;

    /** @brief map of distances to other solutions in the population, see warning for id*/
    std::map<u_long, int> distances;

    /** @brief Header csv*/
    const static std::string header_csv;

    Solution();

    /**
     * @brief Convert a solution under the form of a group of colors
     *
     * @param solution
     */
    Solution(const std::vector<std::vector<int>> &solution);

    /**
     * @brief Convert from an UBQP solution
     *
     */
    Solution(const std::vector<bool> &solution);

    Solution reduce_nb_colors_partial_legal(const int nb_colors) const;
    Solution reduce_nb_colors_illegal(const int nb_colors) const;

    /**
     * @brief Init deltas for tabu col
     *
     */
    void init_deltas();

    /**
     * @brief Init best_improve_conflicts and best_improve_colors for tabu col optimized
     */
    void init_deltas_optimized();

    void init_partial_col_optimized_2();
    // /**
    //  * @brief Init nb free colors for ILSTS
    //  */
    // void init_nb_free_colors();

    /**
     * @brief Init conflicts bias, conflicting edges and edge weights for tabu edge
     */
    void init_edge_weights();

    /**
     * @brief Init possible colors for partial col optimized
     */
    void init_possible_colors();

    /**
     * @brief Remove color to vertices in conflicts to obtain a partial legal solution
     *
     */
    void remove_penalty();

    /**
     * @brief Color vertices with no color to a random color (minimize the number of
     * conflicts, no new colors)
     *
     */
    void color_uncolored();

    /**
     * @brief Remove vertex from its color
     *
     * @param vertex
     */
    void delete_from_color(const int vertex);

    /**
     * @brief Delete the vertex from its old color and move it to the new one
     * and return its old color
     * for tabu col
     */
    int move_to_color(const int vertex, const int new_color);

    /**
     * @brief Delete the vertex from its old color and move it to the new one
     * while updating best_improve_color and best_improve_conflicts
     * and return its old color
     * for tabu col optimized
     */
    int move_to_color_optimized(const int vertex, const int new_color);

    /**
     * @brief Delete the vertex from its old color and move it to the new one
     * while updating best_improve_color and best_improve_conflicts
     * and return its old color
     * for tabu edge
     */
    int move_to_color_edge(const int vertex, const int color);

    /**
     * @brief Add the uncolored vertex to the color
     * -1 for proposed color to create a new color that will be returned
     * for solution initialisation
     */
    int add_to_color(const int vertex, int proposed_color);

    /**
     * @brief Add the vertex to the color while removing its neighbors from the color
     *
     */
    void grenade_move(const int vertex, const int color);

    void grenade_move_optimized_2(const int vertex, const int color);

    /**
     * @brief Grenade move the vertex to the color and its neighbors to other random
     * colors, one neighbor can be uncolored at the end. For partial col optimized
     */
    void grenade_move_optimized(const int vertex, const int color);

    bool check_solution() const;

    /**
     * @brief List the colors where the vertex can be added while adding the least number
     * of conflicts as possible (no new colors)
     */
    std::vector<int> best_possible_colors(const int vertex) const;

    bool is_legal() const;

    int nb_colors() const;

    /**
     * @brief Return the color of the vertex
     */
    int operator[](const int vertex) const;

    int penalty() const;

    const std::vector<int> &uncolored() const;

    int nb_uncolored() const;

    const std::vector<int> &conflicting_vertices() const;

    int best_delta(const int vertex) const;

    const std::vector<int> &best_improve_colors(const int vertex) const;

    int delta_conflicts_colors(const int vertex, const int color) const;

    /**
     * @brief Number of conflicts for the vertex in the current color
     */
    int nb_conflicts(const int vertex) const;

    /**
     * @brief Number of conflits for the vertex in the color
     */
    int nb_conflicts(const int vertex, const int color) const;

    const std::vector<std::vector<int>> &conflicts_colors() const;

    int color_size(const int color) const;

    const std::vector<int> &possible_colors(const int vertex) const;

    const std::vector<int> &colors() const;

    int first_free_vertex() const;

    void increment_first_free_vertex();

    void to_legal();

    /**
     * @brief format to csv
     */
    std::string format() const;
};

/**
 * @brief Compute an approximation of the distance between two solutions
 */
[[nodiscard]] int distance_approximation(const Solution &sol1, const Solution &sol2);
[[nodiscard]] int distance_approximation(const std::vector<int> &sol1,
                                         const std::vector<int> &sol2);

/**
 * @brief Compute the accurate distance between two solutions
 */
[[nodiscard]] int distance_accurate(const Solution &sol1, const Solution &sol2);
[[nodiscard]] int distance_accurate(const std::vector<int> &sol1,
                                    const std::vector<int> &sol2);
