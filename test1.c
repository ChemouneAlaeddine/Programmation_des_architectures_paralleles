#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(){
	//omp_set_num_threads(3);
	#pragma omp parallel
	{
	printf("Bonjour\n");
	//barriere impliicite
	}
	printf("Au revoir\n");
	
	return EXIT_SUCCESS;
}