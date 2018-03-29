
export OMP_NUM_THREADS

ITE=$(seq 10) # nombre de mesures
  
THREADS=$(seq 2 2 12) # nombre de threads

PARAM="./prog -k sable -s 2048 -p alea -i 500 -n -v " # parametres commun à toutes les executions 

execute (){
EXE="$PARAM $*"
OUTPUT="$(echo $* | tr -d ' ')"
for nb in $ITE; do for OMP_NUM_THREADS in $THREADS; do  echo -n "$OMP_NUM_THREADS " >> $OUTPUT ; $EXE 2>> $OUTPUT; done; done
}

#for i in 1 2 3 4 5 ;
#do
#execute seq
#execute seq_double
#execute seq_diagonal
#execute seq_deroule2
#execute seq_deroule3
#execute seq_deroule4
#execute seq_deroule5
#execute omp_for
#execute omp_task
#execute task
execute tiled
#execute tiled_diagonal
execute tiled_task
#execute tiled_for
#execute tiled_double
execute tiled_omp_for
#done
