#!/bin/bash

#SBATCH --job-name=ls2h
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-node=1
#SBATCH --array=1-1000
#SBATCH --time=02:10:00
#SBATCH --partition=SMP-short
#SBATCH --exclude=cribbar[041-056]
#SBATCH --output=/scratch/LERIA/grelier_c/slurm_output/slurm-%x-%a-%j.out
#SBATCH --mem=4G

# increase number of color when solution not found
python3 scripts/one_job.py "${SLURM_ARRAY_TASK_ID}" "$1"

# decrease number of color during search
# python3 scripts/one_job_one_run.py "${SLURM_ARRAY_TASK_ID}" "$1"
