#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#define N 2

double moy(int debut, int fin, int *elements[8]){
	int somme =0;
	for (int i = debut; i <= fin; i++)
	{
		somme += elements[i];
		printf("la somme %d\n",somme);
	}
	return (double)(somme/(fin-debut));
}

struct sequences{
	int debut;
	int fin;
};

int main()
{
  int elements[8] = {0,1,2,3,4,5,10,9};
  double moyennes[N];
  struct sequences seq[2];
  seq[0].debut = 0;
  seq[0].fin = 3;
  seq[1].debut = 5;
  seq[1].fin = 6;

  for (int i = 0; i < N; i++)
  {
  	moyennes[i] = moy(seq->debut,seq->fin,elements);
  }

  printf("moyennes : [");
  for (int i = 0; i < N; i++)
  {
  	printf("%f ,",moyennes[i]);
  }
  printf("]\n");
}
