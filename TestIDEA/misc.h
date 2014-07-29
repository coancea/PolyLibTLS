#include <stdio.h>

/************************
** FUNCTION PROTOTYPES **
************************/

long randwc(long num);

unsigned long abs_randwc(unsigned long num);

long randnum(long lngval);

long randwc(long num)
{
	return(randnum(0L)%num);
}

unsigned long abs_randwc(unsigned long num)
{
long temp;		/* Temporary storage */

temp=randwc(num);
if(temp<0) temp=0L-temp;

return((unsigned long)temp);
}

long randnum(long lngval)
{
	register long interm;
	static long randw[2] = { 13L , 117L };

	if (lngval!=0L)
	{	randw[0]=13L; randw[1]=117L; }

	interm=(randw[0]*254754L+randw[1]*529562L)%999563L;
	randw[1]=randw[0];
	randw[0]=interm;
	return(interm);
}



