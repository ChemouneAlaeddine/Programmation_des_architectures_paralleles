export OMP_NUM_THREADS
export GOMP_CPU_AFFINITY

ITE=$(seq 3) # nombre de mesures
THREADS=" 1 4 8 12" # nombre de threads
GOMP_CPU_AFFINITY=$(seq 0 23 ) # vérifier à l'aide de lstopo la bonne alternance des processeurs

PARAM="../prog -n -k transpose -i 100 -r 100 -s  " # parametres commun à toutes les executions 

execute (){
EXE="$PARAM $*"
OUTPUT="$(echo $* | tr -d ' ')"
for nb in $ITE; do for OMP_NUM_THREADS in $THREADS ; do echo -n "$OUTPUT $OMP_NUM_THREADS " >> ALL ; $EXE 2>> ALL; done; done
}

for i in 256 512 1024 ;
do
	     execute  $i -v omp
	     execute  $i -v omp_tiled
done