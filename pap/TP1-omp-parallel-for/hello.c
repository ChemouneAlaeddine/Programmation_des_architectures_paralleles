#include <stdio.h>
#include <omp.h>

int main()
{
#pragma omp parallel
{
	#pragma omp critical
	{
   printf("Bonjour ! je suis le thread : %d\n",omp_get_thread_num());
   printf("Au revoir !je suis le thread : %d\n",omp_get_thread_num());
}
}
  return 0;
}
