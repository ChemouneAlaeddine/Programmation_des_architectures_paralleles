#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#define N 2
/* macro de mesure de temps, retourne une valeur en µsecondes */
#define TIME_DIFF(t1, t2) \
        ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

struct sequences{
  int debut;
  int fin;
};

double moy(long debut, long fin, int* elements);

int main()
{
  int sum,elements[1000];
  struct timeval t1, t2;
  unsigned long temps;

  #pragma omp parallel for schedule(static)
  for (long i = 0; i < 1000; i++)
  {
    elements[i] = rand()%10;
  }

  double moyennes[100000];
  struct sequences seq[100000];
  sum = 0;
  gettimeofday(&t1,NULL);

  #pragma omp parallel for schedule(static)
  for (long i = 0; i < 100000; i++)
  {
    seq[i].debut = rand()%996;
    seq[i].fin = seq[i].debut+4;
  }

  #pragma omp parallel for schedule(static)
  for (long i = 0; i < 100000; i++)
  {
  	moyennes[i] = (double)moy(seq[i].debut,seq[i].fin,elements);
  }

  printf("moyennes : [");
  #pragma omp parallel for schedule(static)
  for (long i = 0; i < 100000; i++)
  {
  	printf("%.1f",(double)moyennes[i]);
    if (i!=99999)
      printf("|");
  }
  printf("]\n");
  gettimeofday(&t2,NULL);
  temps = TIME_DIFF(t1,t2);
  printf("seq  = %ld.%03ldms   sum = %u\n", temps/1000, temps%1000, sum);
  return 0;
}

double moy(long debut, long fin, int* elements){
  int somme =0;
  for (long i = debut; i <= fin; i++)
  {
    somme += elements[i];
  }
  return (double)somme/(1+fin-debut);
}