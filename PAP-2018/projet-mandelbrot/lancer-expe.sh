
export OMP_NUM_THREADS

ITE=$(seq 5) # nombre de mesures
  
THREADS=$(seq 2 2 8) # nombre de threads

PARAM="./prog -s 1024 -k mandel -v" # parametres commun à toutes les executions 


execute (){
EXE="$PARAM $*"
OUTPUT="$(echo $* | tr -d ' ')"
for nb in $ITE; do for OMP_NUM_THREADS in $THREADS; do  echo -n "$OMP_NUM_THREADS " >> $OUTPUT ; $EXE 2>> $OUTPUT; done; done
}

execute omps
execute ompd
execute omptiled
execute omptask
