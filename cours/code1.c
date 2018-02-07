// INSCRIRE VOTRE NUMERO ICI  

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define DIM 511
#define MAX(a,b) ((a)>(b)?(a):(b))

int image[DIM+1][DIM+1];

void initialiser_image(int pour128)
{
  int i,j ;

  for(j = 0; j<=DIM ; j++) // mettre les bords a 0
    {
      image[0][j]=0;
      image[DIM][j]=0;
    }  
  for(i = 1; i<DIM ; i++) 
    {
      image[i][0]=0;
      for(j = 1; j<DIM ; j++)
	image[i][j]= ((random() & 127) < pour128) ;
      image[i][DIM]=0;
    }
}

void identifier_pixels() // donner a chaque pixel blanc l'indice de sa case
{
  int i,j;
  for(i = 1; i<DIM ; i++)
    for(j = 1; j<DIM ; j++)
      image[i][j] = image[i][j]*(i*DIM+j);
}

int monter_max(int i_d, int j_d, int i_f, int j_f)
{
  int i,j;
  int changement = 0;
  for (i=i_d; i >= i_f; i--)
      for (j=j_d; j >= j_f; j--)
	if (image[i][j])
	  { 
	    int m;
	    m = MAX(image[i+1][j] , image[i][j+1]);
	    if (m > image[i][j])
	      {	    
		changement = 1;
		image[i][j] = m;
	      }
	  }
  return changement;
}














int descendre_max(int i_d, int j_d, int i_f, int j_f)
{
  int i,j;
  int changement = 0;
 

 for (i=i_d; i <= i_f; i++)


    for (j=j_d; j <= j_f; j++)


      { 


	if (image[i][j])

	  {


	    int m = MAX(image[i-1][j] , image[i][j-1]);


	    if (m > image[i][j])


	      {	    


		changement = 1;


		image[i][j] = m;


	      }
	    

	  }

      }


  return changement;

}

void identifier_objets()
{
  identifier_pixels();
  // boucler jusqu'a stagnation
  while((descendre_max(1,1,DIM,DIM) == 1)  | (monter_max(DIM,DIM,1,1) == 1))
    ; 
  }
