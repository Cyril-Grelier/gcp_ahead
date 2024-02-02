"""
this script is used to run one job from the to_run file
the syntax is python one_job.py line_number to_eval_file
the program will run the job at the given line_number in the given to_eval_file

"""
import subprocess
import sys
from typing import Union
import os
import fcntl


def get_line(line_number: int, to_eval_file: str) -> str:
    """this function returns the line at the given line_number in the given file

    Args:
        line_number (int): number of the line to return
        to_eval_file (str): file to read

    Returns:
        str: line at the given line_number in the given file
    """
    with open(to_eval_file, encoding="utf8") as file:
        for i, line in enumerate(file.readlines()):
            if i + 1 == line_number:
                return line


def get_parameters(line: str) -> dict[str, Union[str, int]]:
    """this function returns the parameters of the given line

    Args:
        line (str): line to parse

    Returns:
        dict[str, Union[str, int]]: parameters of the given line
    """
    line_split = line.split()
    i: int = 0
    parameters: dict[str, Union[str, int]] = {}
    while i != len(line_split):
        if line_split[i].startswith("--"):
            if line_split[i + 1].isdigit():
                parameters[line_split[i]] = int(line_split[i + 1])
            else:
                parameters[line_split[i]] = line_split[i + 1]
            i += 2
        else:
            i += 1
    return parameters


def run_job(command: str) -> str:
    """this function runs the given line and returns the output of the program

    Args:
        command (str): command to run

    Returns:
        str: output of the program
    """
    process = subprocess.Popen(
        command.split(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    stdout, stderr = process.communicate()
    if stderr:
        print("error while running job :", stderr.decode())
        print(command)
        sys.exit(1)
    return stdout.decode().strip()


def get_results(output_file: str) -> dict[str, Union[str, int]]:
    """this function returns the results of the given output_file

    Args:
        output_file (str): file to read

    Returns:
        dict[str, Union[str, int]]: results of the given output_file
    """
    with open(output_file, encoding="utf8") as f:
        lines = f.readlines()

    # 3rd line is the csv header
    scores_header = lines[2][:-1].split(",")
    # last line is the time at the end of the search, the line before is the last report of the program
    scores_last_line = lines[-2][:-1].split(",")
    results: dict[str, Union[str, int]] = {}
    for header, result in zip(scores_header, scores_last_line):
        if result.isdigit():
            results[header] = int(result)
        else:
            results[header] = result

    # turn, time, nb_uncolored, penalty, nb_colors, solution
    return results


def check_solution(command: str):
    """this function checks the solution with the given command

    Args:
        command (str): command to run

    Raises:
        Exception: if anything when wrong (colors, penalty, etc.)
    """
    process = subprocess.Popen(
        command.split(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    stdout, stderr = process.communicate()
    if stderr:
        raise Exception(f"error when checking solution : {stderr.decode()}\n{command}")
    if stdout:
        raise Exception(f"error when checking solution : {stdout.decode()}\n{command}")


def main():
    """main function of the script"""
    if len(sys.argv) != 3:
        print("Error : syntax should be python one_job.py line_number to_eval_file")
        sys.exit(1)

    line_number = int(sys.argv[1])
    to_eval_file = sys.argv[2]

    line = get_line(line_number, to_eval_file)
    if not line:
        # if the line is empty
        sys.exit(0)
    parameters: dict[str, Union[str, int]] = get_parameters(line)

    os.chdir("build_release")
    output_file = run_job(line)
    os.chdir("..")
    if not output_file:
        print("error : no output file returned")
        print(line)
        sys.exit(1)
    results: dict[str, Union[str, int]] = get_results(output_file)

    # # if no solution have been found, stop the search
    # if results["nb_uncolored"] != 0 or results["penalty"] != 0:
    #     return

    # # otherwise we launch a new job with one less color

    # # check the solution
    # check_line = f'python3 check_solution.py {parameters["--instance"]} gcp {results["nb_colors"]} {results["solution"]}'
    # # the check solution script is in the instances folder
    # os.chdir("instances")
    # try:
    #     check_solution(check_line)
    # except Exception as excep:
    #     os.chdir("..")
    #     print(excep)
    #     print(line)

    # # the solution is legal, we rename the output file
    # os.chdir("..")
    # new_file_name = "_".join(output_file.split("_")[0:-1]) + ".csv"
    # os.rename(output_file, new_file_name)
    # tbt_file = (
    #     "/".join(output_file.split("/")[0:-1]) + "/tbt/" + output_file.split("/")[-1]
    # )
    # if os.path.exists(tbt_file):
    #     new_tbt_file = "_".join(tbt_file.split("_")[0:-1]) + ".csv"
    #     os.rename(tbt_file, new_tbt_file)

    # # remove old launching file
    # old_file = f"to_run/{parameters['--instance']}_{parameters['--rand_seed']}_{parameters['--nb_colors']+1}_{parameters['--output_directory'].split('/')[-1]}"
    # if os.path.exists(old_file):
    #     os.remove(old_file)

    # # then launch a new job with one less color
    # parameters["--nb_colors"] -= 1
    # new_line = "./gc " + " ".join(f"{k} {v}" for k, v in parameters.items())

    # new_file = f"to_run/{parameters['--instance']}_{parameters['--rand_seed']}_{parameters['--nb_colors']}_{parameters['--output_directory'].split('/')[-1]}"
    # with open(new_file, "a", encoding="utf8") as file:
    #     # wait for availability of the file
    #     # fcntl.flock(file, fcntl.LOCK_EX)
    #     file.write(new_line + "\n")
    #     # fcntl.flock(file, fcntl.LOCK_UN)
    # # os.system(f"sbatch scripts/slurm_rerun.sh {new_file}")
    # stream = os.popen(f"sbatch scripts/slurm_rerun.sh {new_file}")
    # output = stream.read()
    # if not output.startswith("Submitted batch"):
    #     print(output)

    # old to add a color
    # if the solution is not legal, we create a new job with one more color
    if results["nb_uncolored"] != 0 or results["penalty"] != 0:
        # prepare the new job with one more color
        parameters["--nb_colors"] += 1
        new_line = "./gc " + " ".join(f"{k} {v}" for k, v in parameters.items())

        # remove last to_run file
        old_file = f"to_run/{parameters['--instance']}_{parameters['--rand_seed']}_{parameters['--nb_colors']-1}_{parameters['--output_directory'].split('/')[-1]}"
        if os.path.exists(old_file):
            os.remove(old_file)
        # remove last output file
        old_file = (
            "_".join(output_file.split("_")[:-1])
            + "_"
            + str(int(output_file.split("_")[-1].split(".")[0]) - 1)
            + ".csv"
        )
        if os.path.exists(old_file):
            os.remove(old_file)
        # remove last tbt file
        output_file = "/".join(output_file.split("/")[:-1] + ["tbt"])
        output_file += f"/{parameters['--instance']}_{parameters['--rand_seed']}_{parameters['--nb_colors']-2}.csv"
        if os.path.exists(output_file):
            os.remove(output_file)

        # create the new job
        new_file = f"to_run/{parameters['--instance']}_{parameters['--rand_seed']}_{parameters['--nb_colors']}_{parameters['--output_directory'].split('/')[-1]}"
        with open(new_file, "a", encoding="utf8") as file:
            # wait for availability of the file
            # fcntl.flock(file, fcntl.LOCK_EX)
            file.write(new_line + "\n")
            # fcntl.flock(file, fcntl.LOCK_UN)
        # os.system(f"sbatch scripts/slurm_rerun.sh {new_file}")
        stream = os.popen(f"sbatch scripts/slurm_rerun.sh {new_file}")
        output = stream.read()
        if not output.startswith("Submitted batch"):
            print(output)
    else:
        # if the solution is legal, we check it and rename the output file
        check_line = f'python3 check_solution.py {parameters["--instance"]} gcp {results["nb_colors"]} {results["solution"]}'
        # the check solution script is in the instances folder
        os.chdir("instances")
        try:
            check_solution(check_line)
        except Exception as excep:
            os.chdir("..")
            print(excep)
            print(line)
            # there is a problem, we rename the output file
            new_file_name = "_".join(output_file.split("_")[0:-1]) + ".csv.error"
            os.rename(output_file, new_file_name)
            tbt_file = (
                "/".join(output_file.split("/")[0:-1])
                + "/tbt/"
                + output_file.split("/")[-1]
            )
            if os.path.exists(tbt_file):
                new_tbt_file = "_".join(tbt_file.split("_")[0:-1]) + ".csv.error"
                os.rename(tbt_file, new_tbt_file)
            return

        os.chdir("..")
        # remove last to_run file
        old_file = f"to_run/{parameters['--instance']}_{parameters['--rand_seed']}_{parameters['--nb_colors']}_{parameters['--output_directory'].split('/')[-1]}"
        if os.path.exists(old_file):
            os.remove(old_file)

        # the solution is legal, we rename the output file
        new_file_name = "_".join(output_file.split("_")[0:-1]) + ".csv"
        os.rename(output_file, new_file_name)
        tbt_file = (
            "/".join(output_file.split("/")[0:-1])
            + "/tbt/"
            + output_file.split("/")[-1]
        )
        if os.path.exists(tbt_file):
            new_tbt_file = "_".join(tbt_file.split("_")[0:-1]) + ".csv"
            os.rename(tbt_file, new_tbt_file)


if __name__ == "__main__":
    main()
