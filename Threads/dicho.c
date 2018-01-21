#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>     /* atof */
#include <math.h>       /* float_t */


double f(double x)
{       
  return (x-1)*(x-1)-1;
}

double dichotom(double a,double b,double eps)
{
  double c=(a+b)/2;
  double fa=f(a),fc=f(c);
  while (fabs(b-a)>eps)
  {
    if (fa*fc>0)
      a=c;
    else
      b=c;
    c=(a+b)/2;
    fc=f(c);
  }
  return c;
}


int main()
{
  double a=1. , b=10, eps=1e-12;
  for (long i=1;i<1e12;i*=10)
    printf("eps=%e,x*=%1.16f\n",1./i,dichotom(a,b,1./i));
  //for(int i=1;i<11;i++)
  //  printf("%e\n",1./i);
  printf("%e\n",1e12);

  return 0;
}