#!/bin/bash

#SBATCH --time=0:10:00   # walltime
#SBATCH --ntasks=16   # number of processor cores (i.e. tasks)
#SBATCH --nodes=1   # number of nodes
#SBATCH --mem-per-cpu=1024M   # memory per CPU core
#SBATCH -J "quicksort"   # job name
#SBATCH --mail-user=cody.heffner@gmail.com   # email address

# Compatibility variables for PBS. Delete if not needed.
# export PBS_NODEFILE=`/fslapps/fslutils/generate_pbs_nodefile`
# export PBS_JOBID=$SLURM_JOB_ID
# export PBS_O_WORKDIR="$SLURM_SUBMIT_DIR"
# export PBS_QUEUE=batch

# Set the max number of threads to use for programs using OpenMP. Should be <= ppn. Does nothing if the program doesn't use OpenMP.
# export OMP_NUM_THREADS=$SLURM_CPUS_ON_NODE

# LOAD MODULES, INSERT CODE, AND RUN YOUR PROGRAMS HERE
#mpicc quicksort.c -o quicksort -lm
for p in `seq 1 4`; do
	for n in `seq 1 15`; do
		COUNT=$((2**$n))
		PROC=$((2**$p))
		for iter in `seq 1 5`; do
			echo numprocs: $PROC, numelements: $COUNT
			mpirun -np $PROC ./quicksort $COUNT
		done
		echo 
	done
	echo 
done