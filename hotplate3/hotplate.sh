#!/bin/bash

#SBATCH --time=0:05:00   # walltime
#SBATCH --ntasks=16   # number of processor cores (i.e. tasks)
#SBATCH --nodes=1   # number of nodes
#SBATCH --mem-per-cpu=1024M   # memory per CPU core
#SBATCH -J "hotplate"   # job name
#SBATCH --mail-user=cody.heffner@gmail.com   # email address
#SBATCH --mail-type=END

# Compatibility variables for PBS. Delete if not needed.
export PBS_NODEFILE=`/fslapps/fslutils/generate_pbs_nodefile`
export PBS_JOBID=$SLURM_JOB_ID
export PBS_O_WORKDIR="$SLURM_SUBMIT_DIR"
export PBS_QUEUE=batch

# Set the max number of threads to use for programs using OpenMP. Should be <= ppn. Does nothing if the program doesn't use OpenMP.
export OMP_NUM_THREADS=$SLURM_CPUS_ON_NODE

# LOAD MODULES, INSERT CODE, AND RUN YOUR PROGRAMS HERE
mpirun -np 1 ./hotplate3
mpirun -np 2 ./hotplate3
mpirun -np 4 ./hotplate3
mpirun -np 8 ./hotplate3
mpirun -np 16 ./hotplate3
mpirun -np 32 ./hotplate3
