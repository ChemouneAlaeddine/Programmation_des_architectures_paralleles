#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TAILLE_MESSAGE	256

int main(void)
{
	int entreeTube;
	char nomTube[] = "essai.fifo";

	char chaineAEcrire[TAILLE_MESSAGE] = "Bonjour";

	if(mkfifo(nomTube, 0644) != 0) 
	{
		fprintf(stderr, "Impossible de créer le tube nommé.\n");
		exit(EXIT_FAILURE);
	}

	if((entreeTube = open(nomTube, O_WRONLY)) == -1) 
	{
		fprintf(stderr, "Impossible d'ouvrir l'entrée du tube nommé.\n");
		exit(EXIT_FAILURE);
	}

	write(entreeTube, chaineAEcrire, TAILLE_MESSAGE);


	return EXIT_SUCCESS;
}
