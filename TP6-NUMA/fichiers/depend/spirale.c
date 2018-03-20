
void spirale(int x, int y, int pas, int nbtours)
{
  int i=x,j=y, tour;
  for( tour=1; tour <= nbtours; tour++)
    {
      for (; i < x + tour*pas;i++)
	image[i][j]=1;
      for (; j< y + tour*pas+1;j++)
	image[i][j]=1;
      for (; i > x - tour*pas-1 ;i--)
	image[i][j]=1;
      for (; j> y - tour*pas-1;j--)
	image[i][j]=1;
    }

}


void regulier(int xdebut, int xfin, int ydebut, int yfin, int pas, int nbtours)
{
  int i,j;
  int taille = nbtours * pas + 2;
  for (i= xdebut + taille; i < xfin - taille; i+=2*taille)
    for (j= ydebut + taille; j < yfin - taille; j+=2*taille)
      spirale(i,j,pas,nbtours); 
}


void aleatoire(int xdebut, int xfin, int ydebut, int yfin, int pas_max, int nbtours_max, int nbspisrales)
{
  do{
    
    int pas =  rand() % pas_max ;
    if (pas < 2)
      pas = 2;
    int nbtours =  nbtours_max / 4 + 4*(rand() % (nbtours_max / 4));
    int x = xdebut + rand() % (xfin - xdebut) + pas * nbtours + 2;
    int y = ydebut + rand() % (yfin - ydebut) + pas * nbtours + 2;;
    if (x + pas * nbtours + 2 < xfin && y + pas * nbtours + 2 < yfin)
      {
	nbspisrales--;
	spirale(x,y,pas,nbtours);
      }
    
  }while (nbspisrales>0);
}

