#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>

int main()
{
    printf("A par %d \n", omp_get_thread_num());

    printf("B par %d \n", omp_get_thread_num());

    return 0;
}
  
