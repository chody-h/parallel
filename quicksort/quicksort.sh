#mpicc quicksort.c -o quicksort -lm

echo "2 proc, 5 numbers"
mpirun -np 2 ./quicksort 5
echo "4 proc, 5 numbers"
mpirun -np 4 ./quicksort 5
echo "8 proc, 5 numbers"
mpirun -np 8 ./quicksort 5
echo "16 proc, 5 numbers"
mpirun -np 16 ./quicksort 5
