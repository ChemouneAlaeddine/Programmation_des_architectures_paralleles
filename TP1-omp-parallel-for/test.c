#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

int main()
{
  //printf("Bonjour je suis le thread : %d\n",omp_get_thread_num());
  
  #pragma omp parallel for schedule(static,3)
  for (int i = 0; i < 10; i++)
  {
    printf("i = %d je suis le thread : %d\n",i,omp_get_thread_num());
  }
  return 0;
}
