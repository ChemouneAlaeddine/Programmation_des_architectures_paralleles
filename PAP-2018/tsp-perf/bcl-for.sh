
OUTPUT=TASK-3
EXE="./task 15 1234 3"

export OMP_NUM_THREADS

for OMP_NUM_THREADS in $(seq 2 2 24); do  echo -n $OMP_NUM_THREADS " " >> $OUTPUT ; $EXE 2>> $OUTPUT; done
