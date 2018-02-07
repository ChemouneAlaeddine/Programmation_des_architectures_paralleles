
#define NUM_BLOCKS 16
#define BLOCK_SIZE (DIM/NUM_BLOCKS)

// celluled gere les dependances entre blocs pour la descente
// celluled[x][y] == 0 <==> les blocs [x-1][y] et [x][y-1] ne sont pas termines
// celluled[x][y] == 1 <==> un des blocs [x-1][y] et [x][y-1] a termine
// celluled[x][y] == 2 <==> les blocs [x-1][y] et [x][y-1] sont termines
   
volatile int celluled[NUM_BLOCKS][NUM_BLOCKS];

// cellulem gere les dependances entre blocs pour la montee
volatile int cellulem[NUM_BLOCKS][NUM_BLOCKS];

volatile int cont = 1;

void lancer_descente(int i, int j)
{
  celluled[i][j]++;
  if (celluled[i][j] != 2 && i != 0 && j != 0)
    return;  // une dependance n'est pas satisfaite
    
  celluled[i][j]= 0; 
  if(descendre_max(i*BLOCK_SIZE,j*BLOCK_SIZE),(i+1)*BLOCK_SIZE,(j+1)*BLOCK_SIZE)) 
    cont = 1;
  
  if (j < NUM_BLOCKS -1)
    lancer_descente(i,j+1);
  if (i < NUM_BLOCKS - 1)
    lancer_descente(i+1,j);
  
}

void lancer_monte(int i, int j)
{
  cellulem[i][j]++;
  if (cellulem[i][j] != 2 && i != NUM_BLOCKS-1 && j != NUM_BLOCKS-1)
    return ; // une dependance n'est pas satisfaite

  cellulem[i][j]= 0;

  if (monter_max((i+1)*BLOCK_SIZE,(j+1)*BLOCK_SIZE,i*BLOCK_SIZE,j*BLOCK_SIZE))
	cont = 1;
  if (j > 0)
    lancer_monte(i,j-1);
  if (i > 0)
    lancer_monte(i-1,j);
}


void identifier_objets_cell()
{
  identifier_pixels();
 
  while(cont)
    {
      cont = 0; 
      lancer_monte(NUM_BLOCKS-1,NUM_BLOCKS-1);
      lancer_descente(0,0);
    }
}
