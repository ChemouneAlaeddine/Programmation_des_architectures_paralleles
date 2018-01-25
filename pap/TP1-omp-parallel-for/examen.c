#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#define N 2

struct sequences{
  int debut;
  int fin;
};

double moy(int debut, int fin, int* elements){
	int somme =0;
	for (int i = debut; i <= fin; i++)
	{
		somme += elements[i];
	}
	return (double)somme/(1+fin-debut);
}

int main()
{
  printf("ok\n");
  int elements[100];

  for (long i = 0; i < 100; i++)
  {
    elements[i] = rand()%10;
  }
  printf("%d,%d,%d\n",elements[2],elements[5],elements[10]);
  double moyennes[N];
  struct sequences seq[10000000];
  for (long i = 0; i < 10000000; i++)
  {
    seq[i].debut = rand()%10000000;
    seq[i].fin = rand()%(10000001-seq[i].debut)+seq[i].debut;
  }

  for (int i = 0; i < N; i++)
  {
  	moyennes[i] = (double)moy(seq[i].debut,seq[i].fin,elements);
  }

  printf("moyennes : [");
  for (int i = 0; i < N; i++)
  {
  	printf("%f ,",(double)moyennes[i]);
  }
  printf("]\n");

  return 0;
}
