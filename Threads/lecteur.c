#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TAILLE_MESSAGE	256

int main(void)
{
	int sortieTube;
	char nomTube[] = "essai.fifo";

	char chaineALire[TAILLE_MESSAGE];

	if((sortieTube = open ("essai.fifo", O_RDONLY)) == -1) 
	{
		fprintf(stderr, "Impossible d'ouvrir la sortie du tube nommé.\n");
		exit(EXIT_FAILURE);
	}

	read(sortieTube, chaineALire, TAILLE_MESSAGE);
        printf("%s", chaineALire);

	return EXIT_SUCCESS;
}
