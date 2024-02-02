#include "Graph.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fmt/printf.h>
#pragma GCC diagnostic pop

namespace graph_instance {
// graph loaded from the instance
std::unique_ptr<const Graph> graph;
// graph for the UBQP problem
std::unique_ptr<const QGraph> qgraph;
} // namespace graph_instance

Graph::Graph(const std::string &name_,
             const int nb_vertices_,
             const int nb_edges_,
             const std::vector<std::vector<bool>> &adjacency_matrix_,
             const std::vector<std::vector<int>> &neighborhood_,
             const std::vector<int> &degrees_,
             const std::vector<std::pair<int, int>> edge_lists_)
    : name(name_),
      nb_vertices(nb_vertices_),
      nb_edges(nb_edges_),
      adjacency_matrix(adjacency_matrix_),
      neighborhood(neighborhood_),
      degrees(degrees_),
      edges_list(edge_lists_) {
}

void load_graph(const std::string &instance_name) {
    // load the edges and vertices of the graph
    std::ifstream file;
    file.open("../instances/reduced_gcp/" + instance_name + ".col");

    if (!file) {
        throw std::runtime_error(
            fmt::format("Didn't find {} in ../instances/reduced_gcp/ or "
                        "../instances/gcp_reduced/ (if problem == gcp)\n"
                        "Did you run \n\n"
                        "git submodule init\n"
                        "git submodule update\n\n"
                        "before executing the program ?(import instances)\n"
                        "Otherwise check that you are in the build "
                        "directory before executing the program\n",
                        instance_name));
        fmt::print(stderr,
                   "Didn't find {} in ../instances/reduced_gcp/ or "
                   "../instances/gcp_reduced/ (if problem == gcp)\n"
                   "Did you run \n\n"
                   "git submodule init\n"
                   "git submodule update\n\n"
                   "before executing the program ?(import instances)\n"
                   "Otherwise check that you are in the build "
                   "directory before executing the program\n",
                   instance_name);
        exit(1);
    }
    int nb_vertices = 0;
    int nb_edges = 0;
    int v1 = 0;
    int v2 = 0;
    std::vector<std::vector<bool>> adjacency_matrix;
    std::vector<std::vector<int>> neighborhood;
    std::vector<std::pair<int, int>> edges_list;
    std::string first;
    file >> first;
    while (!file.eof()) {
        if (first == "e") {
            file >> v1 >> v2;
            --v1;
            --v2;
            if (v1 > v2) {
                int tmp = v1;
                v1 = v2;
                v2 = tmp;
            }
            if (not adjacency_matrix[v1][v2]) {
                adjacency_matrix[v1][v2] = true;
                adjacency_matrix[v2][v1] = true;
                neighborhood[v1].push_back(v2);
                neighborhood[v2].push_back(v1);
                edges_list.emplace_back(v1, v2);
                ++nb_edges;
            }
        } else if (first == "p") {
            file >> first >> nb_vertices >> nb_edges;
            adjacency_matrix = std::vector<std::vector<bool>>(
                nb_vertices, std::vector<bool>(nb_vertices, false));
            neighborhood =
                std::vector<std::vector<int>>(nb_vertices, std::vector<int>(0));
            edges_list.reserve(nb_edges);
        } else {
            getline(file, first);
        }
        file >> first;
    }
    file.close();

    std::vector<int> degrees(nb_vertices, 0);
    for (int vertex = 0; vertex < nb_vertices; ++vertex) {
        degrees[vertex] = static_cast<int>(neighborhood[vertex].size());
    }
    graph_instance::graph = std::make_unique<Graph>(instance_name,
                                                    nb_vertices,
                                                    nb_edges,
                                                    adjacency_matrix,
                                                    neighborhood,
                                                    degrees,
                                                    edges_list);
}

bool operator==(const Arc &a1, const Arc &a2) {
    return a1.head == a2.head and a1.tail == a2.tail;
}

QGraph::QGraph(const int nb_arc_,
               const std::vector<Arc> arcs_,
               const std::vector<std::vector<int>> &neighborhood_,
               const std::vector<std::vector<int>> &q_matrix_)
    : nb_arc(nb_arc_), arcs(arcs_), neighborhood(neighborhood_), qmatrix(q_matrix_) {
}

void init_UBQP() {

    // compute complementary graph
    const int nb_vertices = graph_instance::graph->nb_vertices;
    std::vector<int> degree(nb_vertices, 0);
    int nb_arc{0};
    std::vector<std::vector<bool>> complementary(nb_vertices,
                                                 std::vector<bool>(nb_vertices, false));

    for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
        for (int vertex2 = vertex1 + 1; vertex2 < nb_vertices; ++vertex2) {
            if (graph_instance::graph->adjacency_matrix[vertex1][vertex2]) {
                continue;
            }
            complementary[vertex1][vertex2] = true;
            complementary[vertex2][vertex1] = true;
            ++degree[vertex1];
            ++degree[vertex2];
            ++nb_arc;
        }
    }

    // orient the complementary

    // incidence_matrix[i][j] == 1 if i is the head
    std::vector<std::vector<int>> incidence_matrix(nb_vertices,
                                                   std::vector<int>(nb_vertices, 0));
    // map_matrix[i][j] = arc number if i is the tail
    std::vector<std::vector<int>> map_matrix(nb_vertices,
                                             std::vector<int>(nb_vertices, -1));
    std::vector<Arc> arcs;
    arcs.reserve(nb_arc);

    int map = 0;
    for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
        for (int vertex2 = vertex1 + 1; vertex2 < nb_vertices; ++vertex2) {
            if (not complementary[vertex1][vertex2]) {
                continue;
            }
            // the hightest degree in the complementary is the tail
            if (degree[vertex1] > degree[vertex2]) {
                // vertex1 is the tail
                // vertex2 is the head
                // vertex1 -> vertex2
                incidence_matrix[vertex1][vertex2] = -1;
                incidence_matrix[vertex2][vertex1] = 1;
                map_matrix[vertex2][vertex1] = map;
                arcs.emplace_back(Arc{vertex1, vertex2});
            } else {
                // vertex1 is the head
                // vertex2 is the tail
                // vertex2 -> vertex1
                incidence_matrix[vertex2][vertex1] = -1;
                incidence_matrix[vertex1][vertex2] = 1;
                map_matrix[vertex1][vertex2] = map;
                arcs.emplace_back(Arc{vertex2, vertex1});
            }
            ++map;
        }
    }
    assert(map == nb_arc);

    // fill the auxiliary graph
    std::vector<std::vector<bool>> auxiliary(nb_arc, std::vector<bool>(nb_arc, false));
    std::vector<std::vector<int>> neighborhood(nb_arc);

    for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
        std::vector<int> input;
        std::vector<int> output;
        for (int vertex2 = 0; vertex2 < nb_vertices; ++vertex2) {
            if (incidence_matrix[vertex1][vertex2] == 1) {
                output.emplace_back(vertex2);
            } else if (incidence_matrix[vertex1][vertex2] == -1) {
                input.emplace_back(vertex2);
            }
        }
        if (not output.empty()) {
            for (size_t i = 0; i < output.size() - 1; ++i) {
                for (size_t j = i + 1; j < output.size(); ++j) {
                    const int neighbor1 = output[i];
                    const int neighbor2 = output[j];
                    // the link between arcs is kept only if
                    // the two heads are not connected
                    if (incidence_matrix[neighbor2][neighbor1] == 0) {
                        const int map1 = map_matrix[vertex1][neighbor1];
                        const int map2 = map_matrix[vertex1][neighbor2];
                        auxiliary[map1][map2] = true;
                        auxiliary[map2][map1] = true;
                        neighborhood[map1].emplace_back(map2);
                        neighborhood[map2].emplace_back(map1);
                    }
                    // else {
                    //     fmt::print("remove link between {} and {}\n",
                    //                map_matrix[vertex1][neighbor1],
                    //                map_matrix[vertex1][neighbor2]);
                    // }
                }
            }
        }

        if (not input.empty()) {
            for (size_t i = 0; i < input.size() - 1; ++i) {
                for (size_t j = i + 1; j < input.size(); ++j) {
                    const int neighbor1 = input[i];
                    const int neighbor2 = input[j];
                    const int map1 = map_matrix[neighbor1][vertex1];
                    const int map2 = map_matrix[neighbor2][vertex1];
                    auxiliary[map1][map2] = true;
                    auxiliary[map2][map1] = true;
                    neighborhood[map1].emplace_back(map2);
                    neighborhood[map2].emplace_back(map1);
                }
            }
        }

        for (const int neighbor1 : input) {
            for (const int neighbor2 : output) {
                const int map1 = map_matrix[vertex1][neighbor2];
                const int map2 = map_matrix[neighbor1][vertex1];
                auxiliary[map1][map2] = true;
                auxiliary[map2][map1] = true;
                neighborhood[map1].emplace_back(map2);
                neighborhood[map2].emplace_back(map1);
            }
        }
    }

    // for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
    //     std::vector<int> input;
    //     std::vector<int> output;
    //     for (int vertex2 = 0; vertex2 < nb_vertices; ++vertex2) {
    //         if (incidence_matrix[vertex1][vertex2] == 1) {
    //             input.emplace_back(vertex2);
    //         } else if (incidence_matrix[vertex1][vertex2] == -1) {
    //             output.emplace_back(vertex2);
    //         }
    //     }
    //     if (not input.empty()) {
    //         for (size_t i = 0; i < input.size() - 1; ++i) {
    //             for (size_t j = i + 1; j < input.size(); ++j) {
    //                 const int neighbor1 = input[i];
    //                 const int neighbor2 = input[j];
    //                 // the link between arcs is kept only if
    //                 // the two heads are not connected
    //                 if (incidence_matrix[neighbor2][neighbor1] == 0) {
    //                     const int map1 = map_matrix[vertex1][neighbor1];
    //                     const int map2 = map_matrix[vertex1][neighbor2];
    //                     auxiliary[map1][map2] = true;
    //                     auxiliary[map2][map1] = true;
    //                     neighborhood[map1].emplace_back(map2);
    //                     neighborhood[map2].emplace_back(map1);
    //                     ++nb_links;
    //                 }
    //                 // else {
    //                 //     fmt::print("remove link between {} and {}\n",
    //                 //                map_matrix[vertex1][neighbor1],
    //                 //                map_matrix[vertex1][neighbor2]);
    //                 // }
    //             }
    //         }
    //     }

    //     if (not output.empty()) {
    //         for (size_t i = 0; i < output.size() - 1; ++i) {
    //             for (size_t j = i + 1; j < output.size(); ++j) {
    //                 const int neighbor1 = output[i];
    //                 const int neighbor2 = output[j];
    //                 const int map1 = map_matrix[neighbor1][vertex1];
    //                 const int map2 = map_matrix[neighbor2][vertex1];
    //                 auxiliary[map1][map2] = true;
    //                 auxiliary[map2][map1] = true;
    //                 neighborhood[map1].emplace_back(map2);
    //                 neighborhood[map2].emplace_back(map1);
    //                 ++nb_links;
    //             }
    //         }
    //     }

    //     for (const int neighbor1 : input) {
    //         for (const int neighbor2 : output) {
    //             const int map1 = map_matrix[neighbor2][vertex1];
    //             const int map2 = map_matrix[vertex1][neighbor1];
    //             auxiliary[map1][map2] = true;
    //             auxiliary[map2][map1] = true;
    //             neighborhood[map1].emplace_back(map2);
    //             neighborhood[map2].emplace_back(map1);
    //             ++nb_links;
    //         }
    //     }
    // }

    // to UBQP
    std::vector<std::vector<int>> qmatrix =
        std::vector<std::vector<int>>(nb_arc, std::vector<int>(nb_arc, 0));
    for (int arc1{0}; arc1 < nb_arc; ++arc1) {
        for (int arc2{0}; arc2 < nb_arc; ++arc2) {
            if (arc1 == arc2) {
                qmatrix[arc1][arc2] = -1;
            } else {
                qmatrix[arc1][arc2] = 2 * static_cast<int>(auxiliary[arc1][arc2]);
            }
        }
    }

    graph_instance::qgraph =
        std::make_unique<QGraph>(nb_arc, arcs, neighborhood, qmatrix);

    if (false) {
        // convert the graph to dot
        double radius = 300;
        std::FILE *file;
        file = std::fopen(("1_" + graph_instance::graph->name + "_original.dot").c_str(),
                          "w");
        fmt::print(file, "\ngraph{{\n");
        for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
            const double x = std::cos(((M_PI * 2) / nb_vertices) * vertex1) * radius;
            const double y = std::sin(((M_PI * 2) / nb_vertices) * vertex1) * radius;
            fmt::print(file, "\t{}[pos=\"{},{}\"]\n", vertex1, x, y);
        }
        for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
            for (int vertex2 = vertex1 + 1; vertex2 < nb_vertices; ++vertex2) {
                if (graph_instance::graph->adjacency_matrix[vertex1][vertex2]) {
                    fmt::print(file, "\t{} -- {}\n", vertex1, vertex2);
                }
            }
        }
        fmt::print(file, "}}\n");
        std::fflush(file);
        std::fclose(file);

        file = std::fopen(
            ("2_" + graph_instance::graph->name + "_complementary.dot").c_str(), "w");
        fmt::print(file, "\ngraph{{\n");
        for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
            const double x = std::cos(((M_PI * 2) / nb_vertices) * vertex1) * radius;
            const double y = std::sin(((M_PI * 2) / nb_vertices) * vertex1) * radius;
            fmt::print(file, "\t{}[pos=\"{},{}\"]\n", vertex1, x, y);
        }
        for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
            for (int vertex2 = vertex1 + 1; vertex2 < nb_vertices; ++vertex2) {
                if (complementary[vertex1][vertex2]) {
                    fmt::print(file, "\t{} -- {}\n", vertex1, vertex2);
                }
            }
        }
        fmt::print(file, "}}\n");
        std::fflush(file);
        std::fclose(file);

        file = std::fopen(
            ("3_" + graph_instance::graph->name + "_map_matrix.dot").c_str(), "w");
        fmt::print(file, "\ndigraph{{\n");
        for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
            const double x = std::cos(((M_PI * 2) / nb_vertices) * vertex1) * radius;
            const double y = std::sin(((M_PI * 2) / nb_vertices) * vertex1) * radius;
            fmt::print(file, "\t{}[pos=\"{},{}\"]\n", vertex1, x, y);
        }
        const std::string colors[] = {"black",
                                      "orange",
                                      "blue1",
                                      "gold1",
                                      "brown",
                                      "purple",
                                      "forestgreen",
                                      "coral1",
                                      "tomato",
                                      "lightskyblue3",
                                      "silver",
                                      "darkseagreen1",
                                      "purple3",
                                      "lightcyan3",
                                      "peru",
                                      "burlywood",
                                      "peachpuff",
                                      "webgrey",
                                      "mistyrose1",
                                      "lightgoldenrod",
                                      "lawngreen",
                                      "red1",
                                      "fuchsia",
                                      "darkorange3",
                                      "seashell4",
                                      "orchid2",
                                      "turquoise",
                                      "aliceblue",
                                      "pink",
                                      "violetred2",
                                      "seagreen2",
                                      "whitesmoke",
                                      "cyan2",
                                      "deeppink",
                                      "orchid4",
                                      "azure4",
                                      "antiquewhite1",
                                      "orangered",
                                      "chocolate1",
                                      "wheat3",
                                      "turquoise3",
                                      "darkgoldenrod1",
                                      "rosybrown1",
                                      "dimgrey",
                                      "chartreuse1",
                                      "goldenrod1",
                                      "ivory4",
                                      "pink3",
                                      "mistyrose4",
                                      "lightcyan",
                                      "palevioletred4",
                                      "lightgoldenrod4",
                                      "chocolate3",
                                      "mediumorchid1",
                                      "navajowhite",
                                      "palegreen",
                                      "khaki2",
                                      "thistle2",
                                      "navyblue",
                                      "mediumslateblue",
                                      "lightyellow2",
                                      "peachpuff2",
                                      "chartreuse",
                                      "lavenderblush1",
                                      "mediumpurple3",
                                      "slategray1",
                                      "sienna2",
                                      "powderblue",
                                      "dimgray",
                                      "linen",
                                      "lightcyan4",
                                      "azure1",
                                      "teal",
                                      "tan4",
                                      "snow4",
                                      "mediumvioletred",
                                      "darkolivegreen",
                                      "lightsalmon1",
                                      "lightgrey",
                                      "darkolivegreen4",
                                      "transparent",
                                      "seagreen3",
                                      "darkgreen",
                                      "blanchedalmond",
                                      "darkturquoise",
                                      "lightgray",
                                      "lightsalmon2",
                                      "magenta3",
                                      "blueviolet",
                                      "rosybrown",
                                      "limegreen",
                                      "bisque",
                                      "lightcyan1",
                                      "aquamarine3",
                                      "deepskyblue3",
                                      "violetred3",
                                      "deeppink1",
                                      "darkseagreen4",
                                      "crimson",
                                      "blue3",
                                      "palevioletred2",
                                      "darkorchid2",
                                      "cadetblue",
                                      "darkslategray1",
                                      "lightpink1",
                                      "orchid1",
                                      "orangered2",
                                      "springgreen3",
                                      "bisque1",
                                      "hotpink",
                                      "floralwhite",
                                      "lightgoldenrodyellow",
                                      "brown3",
                                      "mistyrose2",
                                      "tan2",
                                      "tomato4",
                                      "lightblue",
                                      "darkgoldenrod3",
                                      "slategray",
                                      "cornflowerblue",
                                      "palegoldenrod",
                                      "lightblue4",
                                      "orange2",
                                      "mediumblue",
                                      "olivedrab4",
                                      "paleturquoise1",
                                      "lightsalmon4",
                                      "lightsteelblue3",
                                      "none",
                                      "steelblue4",
                                      "violetred4",
                                      "hotpink3",
                                      "ivory2",
                                      "lightgoldenrod1",
                                      "chartreuse4",
                                      "deepskyblue2",
                                      "deepskyblue1",
                                      "burlywood2",
                                      "chocolate",
                                      "firebrick3",
                                      "papayawhip",
                                      "darkorchid1",
                                      "tan3",
                                      "aquamarine4",
                                      "aquamarine1",
                                      "thistle",
                                      "mistyrose",
                                      "springgreen1",
                                      "rosybrown2",
                                      "olivedrab3",
                                      "royalblue4",
                                      "lightskyblue4",
                                      "mediumaquamarine",
                                      "skyblue4",
                                      "ghostwhite",
                                      "salmon1",
                                      "mediumorchid",
                                      "lightyellow3",
                                      "darkslategray2",
                                      "darkgrey",
                                      "turquoise2",
                                      "lightsalmon",
                                      "thistle4",
                                      "darkolivegreen1",
                                      "royalblue2",
                                      "wheat4",
                                      "dodgerblue1",
                                      "orangered3",
                                      "antiquewhite",
                                      "maroon4",
                                      "cornsilk4",
                                      "x11maroon",
                                      "olivedrab2",
                                      "pink4",
                                      "khaki",
                                      "palevioletred",
                                      "seashell3",
                                      "slateblue3",
                                      "orange4",
                                      "turquoise4",
                                      "lightskyblue",
                                      "olivedrab1",
                                      "lightcoral",
                                      "lightgoldenrod3",
                                      "goldenrod4",
                                      "aquamarine2",
                                      "hotpink2",
                                      "khaki1",
                                      "dodgerblue3",
                                      "lightgoldenrod2",
                                      "brown1",
                                      "dodgerblue",
                                      "maroon3",
                                      "coral3",
                                      "cornsilk1",
                                      "invis",
                                      "lightblue1",
                                      "lightslategray",
                                      "lightpink2",
                                      "darkslategray3",
                                      "white",
                                      "wheat1",
                                      "deepskyblue",
                                      "slategray3",
                                      "lavenderblush2",
                                      "hotpink1",
                                      "thistle1",
                                      "lightskyblue1",
                                      "lavenderblush",
                                      "darkolivegreen3",
                                      "seagreen4",
                                      "plum2",
                                      "gainsboro",
                                      "snow3",
                                      "goldenrod2",
                                      "lemonchiffon",
                                      "slategray2",
                                      "turquoise1",
                                      "tan",
                                      "purple2",
                                      "yellow2",
                                      "tomato1",
                                      "goldenrod",
                                      "mediumorchid3",
                                      "lemonchiffon1",
                                      "webmaroon",
                                      "paleturquoise3",
                                      "slateblue4",
                                      "x11purple",
                                      "hotpink4",
                                      "sienna3",
                                      "yellow",
                                      "antiquewhite4",
                                      "navy",
                                      "mistyrose3",
                                      "plum3",
                                      "brown4",
                                      "dodgerblue4",
                                      "darkslategray",
                                      "darkseagreen2",
                                      "cadetblue1",
                                      "springgreen2",
                                      "darkred",
                                      "slateblue",
                                      "coral4",
                                      "gray",
                                      "darkorange2",
                                      "darkgoldenrod",
                                      "wheat2",
                                      "gold3",
                                      "antiquewhite3",
                                      "orangered4",
                                      "cadetblue3",
                                      "lightgreen",
                                      "aquamarine",
                                      "olivedrab",
                                      "navajowhite3",
                                      "cyan",
                                      "mediumorchid2",
                                      "red4",
                                      "blue2",
                                      "plum4",
                                      "darkorchid3",
                                      "midnightblue",
                                      "sienna4",
                                      "cadetblue2",
                                      "indianred4",
                                      "khaki4",
                                      "burlywood4",
                                      "bisque3",
                                      "ivory1",
                                      "skyblue",
                                      "magenta",
                                      "firebrick2",
                                      "sienna",
                                      "rebeccapurple",
                                      "magenta1",
                                      "cyan1",
                                      "violet",
                                      "plum1",
                                      "lemonchiffon2",
                                      "firebrick",
                                      "lightblue2",
                                      "red",
                                      "olive",
                                      "orange1",
                                      "darkcyan",
                                      "honeydew3",
                                      "orchid3",
                                      "goldenrod3",
                                      "steelblue1",
                                      "palegreen2",
                                      "palevioletred1",
                                      "brown2",
                                      "beige",
                                      "saddlebrown",
                                      "cyan4",
                                      "darkkhaki",
                                      "maroon2",
                                      "mediumpurple",
                                      "steelblue",
                                      "firebrick4",
                                      "cornsilk2",
                                      "paleturquoise2",
                                      "purple1",
                                      "maroon",
                                      "cadetblue4",
                                      "steelblue2",
                                      "royalblue1",
                                      "peachpuff3",
                                      "bisque4",
                                      "khaki3",
                                      "skyblue2",
                                      "cornsilk3",
                                      "darkslategrey",
                                      "indigo",
                                      "mediumorchid4",
                                      "salmon4",
                                      "x11grey",
                                      "seagreen1",
                                      "lavenderblush3",
                                      "royalblue",
                                      "snow2",
                                      "deeppink3",
                                      "thistle3",
                                      "x11gray",
                                      "darkmagenta",
                                      "red3",
                                      "webgreen",
                                      "darkgoldenrod2",
                                      "darkgray",
                                      "yellow1",
                                      "violetred1",
                                      "maroon1",
                                      "slategrey",
                                      "lightsalmon3",
                                      "lightslategrey",
                                      "sandybrown",
                                      "yellowgreen",
                                      "coral2",
                                      "burlywood1",
                                      "salmon",
                                      "mediumturquoise",
                                      "yellow3",
                                      "pink2",
                                      "seashell1",
                                      "springgreen",
                                      "magenta4",
                                      "cornsilk",
                                      "slateblue2",
                                      "lightyellow4",
                                      "navajowhite4",
                                      "darkslategray4",
                                      "lightyellow1",
                                      "darkblue",
                                      "lemonchiffon3",
                                      "seagreen",
                                      "gold2",
                                      "x11green",
                                      "chartreuse3",
                                      "lightslateblue",
                                      "snow",
                                      "ivory3",
                                      "azure3",
                                      "chocolate4",
                                      "purple4",
                                      "indianred",
                                      "tan1",
                                      "indianred3",
                                      "mintcream",
                                      "lightpink",
                                      "chocolate2",
                                      "magenta2",
                                      "mediumspringgreen",
                                      "lightblue3",
                                      "pink1",
                                      "seashell2",
                                      "salmon2",
                                      "cyan3",
                                      "paleturquoise4",
                                      "springgreen4",
                                      "coral",
                                      "violetred",
                                      "navajowhite2",
                                      "palegreen1",
                                      "orangered1",
                                      "orchid",
                                      "darkolivegreen2",
                                      "ivory",
                                      "plum",
                                      "palevioletred3",
                                      "lavender",
                                      "mediumpurple1",
                                      "antiquewhite2",
                                      "lightskyblue2",
                                      "mediumpurple2",
                                      "gold4",
                                      "tomato3",
                                      "azure",
                                      "deepskyblue4",
                                      "honeydew1",
                                      "darkorange4",
                                      "orange3",
                                      "paleturquoise",
                                      "lightcyan2",
                                      "lightsteelblue2",
                                      "lightsteelblue1",
                                      "peachpuff1",
                                      "moccasin",
                                      "webgray",
                                      "slategray4",
                                      "rosybrown4",
                                      "darkorange",
                                      "darkorchid4",
                                      "steelblue3",
                                      "azure2",
                                      "snow1",
                                      "salmon3",
                                      "deeppink4",
                                      "mediumpurple4",
                                      "tomato2",
                                      "indianred2",
                                      "darksalmon",
                                      "lightseagreen",
                                      "gold",
                                      "deeppink2",
                                      "red2",
                                      "dodgerblue2",
                                      "honeydew",
                                      "peachpuff4",
                                      "navajowhite1",
                                      "yellow4",
                                      "firebrick1",
                                      "palegreen4",
                                      "darkgoldenrod4",
                                      "skyblue1",
                                      "skyblue3",
                                      "lavenderblush4",
                                      "honeydew4",
                                      "palegreen3",
                                      "darkseagreen",
                                      "mediumseagreen",
                                      "lightyellow",
                                      "aqua",
                                      "indianred1",
                                      "lightsteelblue",
                                      "blue4",
                                      "honeydew2",
                                      "lightpink3",
                                      "darkorange1",
                                      "darkseagreen3",
                                      "rosybrown3",
                                      "darkorchid",
                                      "lightsteelblue4",
                                      "darkviolet",
                                      "burlywood3",
                                      "webpurple",
                                      "wheat",
                                      "sienna1",
                                      "lemonchiffon4",
                                      "bisque2",
                                      "lightpink4",
                                      "lime",
                                      "seashell",
                                      "blue",
                                      "darkslateblue",
                                      "oldlace",
                                      "chartreuse2",
                                      "royalblue3",
                                      "slateblue1"};

        for (int vertex1 = 0; vertex1 < nb_vertices; ++vertex1) {
            for (int vertex2 = 0; vertex2 < nb_vertices; ++vertex2) {
                if (incidence_matrix[vertex1][vertex2] == 1) {
                    fmt::print(
                        file,
                        "\t{} -> {} "
                        "[label=\"{}\",fontcolor=\"{}\",color=\"{}\",style=filled]\n",
                        vertex2,
                        vertex1,
                        map_matrix[vertex1][vertex2],
                        colors[map_matrix[vertex1][vertex2]],
                        colors[map_matrix[vertex1][vertex2]]);
                }
            }
        }
        fmt::print(file, "}}\n");
        std::fflush(file);
        std::fclose(file);
        radius = 500;
        file = std::fopen(("4_" + graph_instance::graph->name + "_auxiliary.dot").c_str(),
                          "w");
        fmt::print(file, "\ngraph{{\n");
        for (int vertex1 = 0; vertex1 < nb_arc; ++vertex1) {
            const double x = std::cos(((M_PI * 2) / nb_arc) * vertex1) * radius;
            const double y = std::sin(((M_PI * 2) / nb_arc) * vertex1) * radius;
            fmt::print(file,
                       "\t{}[pos=\"{},{}\",color=\"{}\"]\n",
                       vertex1,
                       x,
                       y,
                       colors[vertex1]);
        }
        for (int vertex1 = 0; vertex1 < nb_arc; ++vertex1) {
            for (int vertex2 = vertex1 + 1; vertex2 < nb_arc; ++vertex2) {
                if (auxiliary[vertex1][vertex2]) {
                    fmt::print(file, "\t{} -- {}\n", vertex1, vertex2);
                }
            }
        }
        fmt::print(file, "}}\n");
        std::fflush(file);
        std::fclose(file);
    }
}
