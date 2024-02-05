# gcp_ahead

AHEAD - Adaptive HEAD algorithm for the graph coloring problem (GCP)

This project is a C++ implementation of the AHEAD algorithm for the graph coloring problem (GCP).
The AHEAD algorithm is a memetic algorithm that uses hyperheuristics to adapt the search to the instance.
This work is related to :

- the code : https://github.com/Cyril-Grelier/wvcp_ahead (same algorithm for the weighted vertex coloring problem)
- the article : TBA

## Requirements

To compile this project you need :

- `cmake 3.14+ <https://cmake.org/>`\_\_
- gcc/g++ 11+
- `Python 3.9+ <https://www.python.org/>`\_\_ (for the slurm jobs, data analysis and documentation)
- `pytorch` :

```bash
    mkdir thirdparty
    cd thirdparty
    wget https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-1.13.0%2Bcpu.zip
    unzip libtorch-shared-with-deps-1.13.0+cpu.zip

    cd ..
    # Load the instances
    git submodule init
    git submodule update
```

You can also use docker :

    docker build -t img_gcc_12 .

    docker run --name gc_ma -v "$(pwd)":/app -it img_gcc_12

# Methods and parameters

The program set the different parameters through a json file.
The main element is the method you want to launch, then all parameters are fixed.

Some parameters are given through the command line interface :

- instance file : `-i` or `--instance`
- number of color to use : `-k` or `--nb_colors`
- does the method start with an infinite number of color and stop when the number of color is reached (`false`) or does the method start with `k` colors and stop when a legal solution is found (`true`) : `-T` or `--use_target`
- random seed : `-r` or `--rand_seed`
- runtime of the job : `-t` or `--time_limit`
- output directory, must be created before launching the job (the memetic algorithm require one more repertory `tbt`(turn by turn), inside the output directory): `-o` or `--output_directory`

For the choice of the method, you can use the JSON files in `parameters` directory.
Methods are divided in 4 categories :

- greedy : greedy algorithms
- local_search : local search algorithms
- mcts : monte carlo tree search algorithms
- memetic : memetic algorithms
