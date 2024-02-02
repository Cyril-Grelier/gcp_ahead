"""
Generate to_eval file which list all run to perform for local search
"""

import os

parameters = [
    # # greedy
    # ("random", "parameters/greedy/random.json"),
    # ("constrained", "parameters/greedy/constrained.json"),
    # ("deterministic", "parameters/greedy/deterministic.json"),
    # ("deterministic_2", "parameters/greedy/deterministic_2.json"),
    # ("adaptive", "parameters/greedy/adaptative.json"),
    # ("DSatur", "parameters/greedy/DSatur.json"),
    # ("RLF", "parameters/greedy/RLF.json"),
    # local search
    # ("tabu_bucket", "parameters/local_search/tabu_bucket.json"),
    # ("partial_col", "parameters/local_search/partial_col.json"),
    # ("partial_col_optimized", "parameters/local_search/partial_col_optimized.json"),
    # ("partial_ts", "parameters/local_search/partial_ts.json"),
    # ("tabu_col", "parameters/local_search/tabu_col.json"),
    # ("tabu_col_optimized", "parameters/local_search/tabu_col_optimized.json"),
    # # memetic
    # ("head", "parameters/memetic/head.json"),
    # ("macol", "parameters/memetic/macol.json"),
    # ("deleter", "parameters/memetic/macol_deleter.json"),
    # ("pursuit", "parameters/memetic/macol_pursuit.json"),
    # ("roulette", "parameters/memetic/macol_roulette.json"),
    # ("neural_net", "parameters/memetic/macol_neural_net.json"),
    # ("random", "parameters/memetic/macol_random.json"),
    # ("ma_random", "parameters/memetic/macol_random.json"),
    # ("ma_ucb", "parameters/memetic/macol_ucb.json"),
    # ("ucb_0.01", "parameters/memetic/macol_ucb_0.01.json"),
    # ("ucb_0.001", "parameters/memetic/macol_ucb_0.001.json"),
    # ("ucb_0.005", "parameters/memetic/macol_ucb_0.005.json"),
    # ("ucb_0.0075", "parameters/memetic/macol_ucb_0.0075.json"),
    # ("bgpx_pco", "parameters/memetic/macol_bgpx_pco.json"),
    # ("bgpx_pco", "parameters/memetic/macol_bgpx_pco.json"),
    # ("bgpx_tco", "parameters/memetic/macol_bgpx_tco.json"),
    # ("gpx_pco", "parameters/memetic/macol_gpx_pco.json"),
    # ("gpx_pco", "parameters/memetic/macol_gpx_pco.json"),
    # ("gpx_tco", "parameters/memetic/macol_gpx_tco.json"),
    # ("random", "parameters/mcts/mcts_greedy_random.json"),
    # ("constrained", "parameters/mcts/mcts_greedy_constrained.json"),
    # ("deterministic", "parameters/mcts/mcts_greedy_deterministic.json"),
    # ("adaptive", "parameters/mcts/mcts_greedy_adaptive.json"),
    # ("dsatur", "parameters/mcts/mcts_greedy_dsatur.json"),
    # ("rlf", "parameters/mcts/mcts_greedy_rlf.json"),
    # ("mcts_tco_0.001", "parameters/mcts/mcts_tco_0.001.json"),
    # ("mcts_tco_0.005", "parameters/mcts/mcts_tco_0.005.json"),
    # ("mcts_tco_0.01", "parameters/mcts/mcts_tco_0.01.json"),
    # ("mcts_pc", "parameters/mcts/mcts_pc.json"),
    # ("mcts_pco", "parameters/mcts/mcts_pco.json"),
    # ("mcts_pco", "parameters/mcts/mcts_pco.json"),
    # ("head_random", "parameters/memetic/head_random.json"),
    # ("head_deleter", "parameters/memetic/head_deleter.json"),
    # ("head_roulette_wheel", "parameters/memetic/head_roulette_wheel.json"),
    # ("head_pursuit", "parameters/memetic/head_pursuit.json"),
    # ("head_ucb", "parameters/memetic/head_ucb.json"),
    # ("head_neural_net", "parameters/memetic/head_neural_net.json"),
    # ("macol_random", "parameters/memetic/macol_random.json"),
    # ("macol_ucb", "parameters/memetic/macol_ucb.json"),
    # ("macol_roulette_wheel", "parameters/memetic/macol_roulette_wheel.json"),
    # ("macol_pursuit", "parameters/memetic/macol_pursuit.json"),
    # ("macol_neural_net", "parameters/memetic/macol_neural_net.json"),
    # ("macol_deleter", "parameters/memetic/macol_deleter.json"),
    # ("head_pc", "parameters/memetic/head_pc.json"),
    # ("head_pco", "parameters/memetic/head_pco.json"),
    # ("head_pts", "parameters/memetic/head_pts.json"),
    # ("head_tco", "parameters/memetic/head_tco.json"),
    # ("head_tco_0.001", "parameters/memetic/head_tco_ls_0.001.json"),
    # ("head_tco_0.01", "parameters/memetic/head_tco_ls_0.01.json"),
    # ("head_tco_0.1", "parameters/memetic/head_tco_ls_0.1.json"),
    # ("head_tco_0.005", "parameters/memetic/head_tco_ls_0.005.json"),
    # ("head_tco_0.05", "parameters/memetic/head_tco_ls_0.05.json"),
    # ("head_pc_0.001", "parameters/memetic/head_pc_ls_0.001.json"),
    # ("head_pc_0.01", "parameters/memetic/head_pc_ls_0.01.json"),
    # ("head_pc_0.1", "parameters/memetic/head_pc_ls_0.1.json"),
    # ("head_pc_0.005", "parameters/memetic/head_pc_ls_0.005.json"),
    # ("head_pc_0.05", "parameters/memetic/head_pc_ls_0.05.json"),
    #
    ("ahead_random", "parameters/memetic/ahead_random.json"),
    ("ahead_deleter", "parameters/memetic/ahead_deleter.json"),
    ("ahead_roulette", "parameters/memetic/ahead_roulette.json"),
    ("ahead_ucb", "parameters/memetic/ahead_ucb.json"),
    ("ahead_pursuit", "parameters/memetic/ahead_pursuit.json"),
    ("ahead_nn", "parameters/memetic/ahead_nn.json"),
]

nb_colors = {
    # "C2000.5": 250,
    # "C2000.9": 550,
    # "C4000.5": 350,
    # "DSJC500.1": 15,
    # "DSJC500.5": 55,
    # "DSJC500.9": 150,
    # "DSJC1000.1": 24,
    # "DSJC1000.5": 108,
    # "DSJC1000.9": 240,
    # "DSJR500.5": 130,
    # "flat300_28_0": 40,
    # "flat1000_50_0": 65,
    # "flat1000_60_0": 65,
    # "flat1000_76_0": 90,
    # "latin_square_10": 115,
    # "le450_25c": 29,
    # "le450_25d": 29,
    # "queen11_11": 14,
    # "queen12_12": 14,
    # "queen13_13": 16,
    # "queen14_14": 17,
    # "r250.5": 69,
    # "r1000.1c": 150,
    # "r1000.5": 260,
    # "wap01a": 48,
    # "wap02a": 48,
    # "wap03a": 48,
    # "wap04a": 48,
    # "wap06a": 48,
    # "wap07a": 48,
    # "wap08a": 48,
    #
    "C2000.5": 146,
    "C2000.9": 403,
    "C4000.5": 277,
    "DSJC500.1": 11,
    "DSJC500.5": 47,
    "DSJC500.9": 125,
    "DSJC1000.1": 19,
    "DSJC1000.5": 81,
    "DSJC1000.9": 221,
    "DSJR500.5": 122,
    "flat300_28_0": 28,
    "flat1000_50_0": 50,
    "flat1000_60_0": 60,
    "flat1000_76_0": 80,
    "latin_square_10": 99,
    "le450_25c": 25,
    "le450_25d": 25,
    "queen11_11": 11,
    "queen12_12": 12,
    "queen13_13": 13,
    "queen14_14": 14,
    "r250.5": 65,
    "r1000.1c": 98,
    "r1000.5": 234,
    "wap01a": 41,
    "wap02a": 40,
    "wap03a": 40,
    "wap04a": 40,
    "wap06a": 40,
    "wap07a": 40,
    "wap08a": 40,
}

nb_iter = {
    "C2000.5": 1200,
    "C2000.9": 1200,
    "C4000.5": 750,
    "DSJC500.1": 3600,
    "DSJC500.5": 3600,
    "DSJC500.9": 3600,
    "DSJC1000.1": 1800,
    "DSJC1000.5": 1800,
    "DSJC1000.9": 1800,
    "DSJR500.5": 3600,
    "flat300_28_0": 3600,
    "flat1000_50_0": 1800,
    "flat1000_60_0": 1800,
    "flat1000_76_0": 1800,
    "latin_square_10": 3600,
    "le450_25c": 3600,
    "le450_25d": 3600,
    "queen11_11": 3600,
    "queen12_12": 3600,
    "queen13_13": 3600,
    "queen14_14": 3600,
    "r250.5": 3600,
    "r1000.1c": 3600,
    "r1000.5": 3600,
    "wap01a": 1800,
    "wap02a": 1800,
    "wap03a": 750,
    "wap04a": 750,
    "wap06a": 3600,
    "wap07a": 1800,
    "wap08a": 1800,
}

instances = nb_colors.keys()

# with open("instances/instance_list_gcp.txt", "r", encoding="UTF8") as file:
#     instances = [line[:-1] for line in file.readlines()]


rand_seeds = list(range(20))

time_limit = 3600 * 3
use_target = "true"  # "true" "false"


output_directory = "/scratch/LERIA/grelier_c/gcp_hard_ahead_hh"
# output_directory = "/app/gcp_hard_ahead_ls"

# os.mkdir(f"{output_directory}/")

for parameter in parameters:
    os.mkdir(f"{output_directory}/{parameter[0]}")
    os.mkdir(f"{output_directory}/{parameter[0]}/tbt")

with open("to_eval_hh", "w", encoding="UTF8") as file:
    for parameter in parameters:
        for instance in instances:
            # nb_color = -1
            nb_color = nb_colors[instance]
            for rand_seed in rand_seeds:
                file.write(
                    f"./gc"
                    f" --instance {instance}"
                    f" --nb_colors {nb_color}"
                    f" --use_target {use_target}"
                    f" --rand_seed {rand_seed}"
                    f" --time_limit {time_limit}"
                    f" --nb_iterations {nb_iter[instance]}"
                    f" --parameters ../{parameter[1]}"
                    f" --output_directory {output_directory}/{parameter[0]}"
                    "\n"
                )
