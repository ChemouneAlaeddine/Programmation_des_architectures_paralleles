#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main()
{
	#pragma omp parallel
	{
		#pragma omp critical
		{
			printf("Bonjour ! je ss %d\n",omp_get_thread_num());
			printf("Au revoir !\n");
		}
	}
	return 0;
}
