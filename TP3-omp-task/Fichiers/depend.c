#include <stdio.h>
#include <stdlib.h>

#define T 10

int A[T][T+1];
int k = 0;

void tache(int i,int j){
	volatile int x = random() % 1000000;
	for (int z = 0; z < x; z++);
	#pragma omp atomic capture
	A[i][j] = k++;
}

int main (int argc, char **argv){
	int i,j;

	#pragma omp parallel
	#pragma omp single
	for (i=0; i < T; i++ )
		for (j=0; j < T; j++ ){
			if(i!=0 && j!=0){
			#pragma omp task firstprivate(i,j) depend(in:A[i-1][j],A[i][j-1])
			tache(i,j);
			#pragma omp task depend(out:A[i][j])
		}else{
			if(i==0 && j==0){
			#pragma omp task firstprivate(i,j)
			tache(i,j);
			}else{
				if(i==0){
					#pragma omp task firstprivate(i,j)
					#pragma omp task depend(in:A[i-1][j])
					tache(i,j);
					#pragma omp task depend(out:A[i][j])
				}else{
					#pragma omp task firstprivate(i,j)
					#pragma omp task depend(in:A[i][j-1])
					tache(i,j);
					#pragma omp task depend(out:A[i][j])
				}
			}
		}
		}
	for (i=0; i < T; i++ ){
		puts("");
		for (j=0; j < T; j++ )
			printf(" %2d ",A[i][j]);
	}
	puts("\n");
}