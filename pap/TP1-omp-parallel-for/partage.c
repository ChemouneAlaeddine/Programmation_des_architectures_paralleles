#include <stdlib.h>
#include <stdio.h>
#include <omp.h>


int
main()
{
  
  int sum = 0;
  int t[100000];

#pragma omp parallel
{
  int i;
  int ma_somme = 0;
  #pragma omp for schedule(static,1)
  for(i = 0; i < 100000; i++)
    ma_somme += t[i];

  #pragma omp critical
  sum += ma_somme;
 }
 #pragma omp barrier
 printf("nbthreads x 100000 = %d\n ",sum);
 return 0;
}
