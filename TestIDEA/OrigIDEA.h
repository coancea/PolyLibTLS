#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>


#include "misc.h"


//unsigned long abs_randwc(unsigned long num);	/* From MISC */
//long randnum(long lngval);

/********************
** IDEA ENCRYPTION **
********************/

typedef unsigned char u8;       /* Unsigned 8-bits */
typedef unsigned short u16;     /* Unsigned 16 bits */
typedef unsigned long u32;      /* Unsigned 32 bits */

/*
** DEFINES
*/
#if(VER==1)

#define IDEAKEYSIZE 16
#define IDEABLOCKSIZE 8
#define ROUNDS 1024*1024*8//32 //8
#define KEYLEN (6*ROUNDS+4)
#define ARRAY_SIZE ROUNDS*2

#else

#define IDEAKEYSIZE 16
#define IDEABLOCKSIZE 8
#define ROUNDS 1024
#define KEYLEN (6*ROUNDS+4)
#define ARRAY_SIZE ROUNDS*ROUNDS/2

#endif

/*
** MACROS
*/
#define low16(x) ((x) & 0x0FFFF)
#define MUL(x,y) (x=mul(low16(x),y))


typedef u16 IDEAkey[KEYLEN];
typedef u16* IDEAKeyPiece;


/********
** mul **
*********
** Performs multiplication, modulo (2**16)+1.  This code is structured
** on the assumption that untaken branches are cheaper than taken
** branches, and that the compiler doesn't schedule branches.
*/
static u16 mul(register u16 a, register u16 b)
{
register u32 p;
if(a)
{       if(b)
        {       p=(u32)(a*b);
                b=low16(p);
                a=(u16)(p>>16);
                return(b-a+(b<a));
        }
        else
                return(1-a);
}
else
        return(1-b);
}


/****************
** en_key_idea **
*****************
** Compute IDEA encryption subkeys Z
*/
static void en_key_idea(u16 *userkey, u16 *Z)
{
int i,j;

/*
** shifts
*/
for(j=0;j<8;j++)
        Z[j]=*userkey++;
for(i=0;j<KEYLEN;j++)
{       i++;
        Z[i+7]=(Z[i&7]<<9)| (Z[(i+1) & 7] >> 7);
        Z+=i&8;
        i&=7;
}
return;
}



/********
** inv **
*********
** Compute multiplicative inverse of x, modulo (2**16)+1
** using Euclid's GCD algorithm.  It is unrolled twice
** to avoid swapping the meaning of the registers.  And
** some subtracts are changed to adds.
*/
static u16 inv(u16 x)
{
u16 t0, t1;
u16 q, y;

if(x<=1)
        return(x);      /* 0 and 1 are self-inverse */
t1=0x10001 / x;
y=0x10001 % x;
if(y==1)
        return(low16(1-t1));
t0=1;
do {
        q=x/y;
        x=x%y;
        t0+=q*t1;
        if(x==1) return(t0);
        q=y/x;
        y=y%x;
        t1+=q*t0;
} while(y!=1);
return(low16(1-t1));
}


/****************
** de_key_idea **
*****************
** Compute IDEA decryption subkeys DK from encryption
** subkeys Z.
*/
IDEAkey TT;
static void de_key_idea(IDEAkey Z, IDEAkey DK)
{
//IDEAkey TT;
int j;
u16 t1, t2, t3;
u16 *p;
p=(u16 *)(TT+KEYLEN);


t1=inv(*Z++);
t2=-*Z++;
t3=-*Z++;
*--p=inv(*Z++);
*--p=t3;
*--p=t2;
*--p=t1;

for(j=1;j<ROUNDS;j++)
{       t1=*Z++;
        *--p=*Z++;
        *--p=t1;
        t1=inv(*Z++);
        t2=-*Z++;
        t3=-*Z++;
        *--p=inv(*Z++);
        *--p=t2;
        *--p=t3;
        *--p=t1;
}
t1=*Z++;
*--p=*Z++;
*--p=t1;
t1=inv(*Z++);
t2=-*Z++;
t3=-*Z++;
*--p=inv(*Z++);
*--p=t3;
*--p=t2;
*--p=t1;
/*
** Copy and destroy temp copy
*/
for(j=0,p=TT;j<KEYLEN;j++)
{       *DK++=*p;
        *p++=0;
}

return;
}

template<class MM, class TH>
static void de_key_idea_spec(IDEAkey Z, IDEAkey DK, MM* mem, TH* th)
{
//IDEAkey TT;
int j;
u16 t1, t2, t3;
u16 *p;
p=(u16 *)(TT+KEYLEN);


t1=inv(*Z++);
t2=-*Z++;
t3=-*Z++;
*--p=inv(*Z++);
*--p=t3;
*--p=t2;
*--p=t1;

for(j=1;j<ROUNDS;j++)
{       
		de_key_idea_iter_spec(Z+6*(j-1), p-6*(j-1), mem, th);
}
t1=*Z++;
*--p=*Z++;
*--p=t1;
t1=inv(*Z++);
t2=-*Z++;
t3=-*Z++;
*--p=inv(*Z++);
*--p=t3;
*--p=t2;
*--p=t1;
/*
** Copy and destroy temp copy
*/
for(j=0,p=TT;j<KEYLEN;j++)
{       *DK++=*p;
        *p++=0;
}

return;
}


/****************
** cipher_idea **
*****************
** IDEA encryption/decryption algorithm.
*/
static void cipher_idea(u16 in[4],
                u16 out[4],
                IDEAkey Z)
{
register u16 x1, x2, x3, x4, t1, t2;
/* register u16 t16;
register u16 t32; */
int r=ROUNDS;

x1=*in++;
x2=*in++;
x3=*in++;
x4=*in;

do {
        MUL(x1,*Z++);
        x2+=*Z++;
        x3+=*Z++;
        MUL(x4,*Z++);

        t2=x1^x3;
        MUL(t2,*Z++);
        t1=t2+(x2^x4);
        MUL(t1,*Z++);
        t2=t1+t2;

        x1^=t1;
        x4^=t2;
        
        t2^=x2;
        x2=x3^t1;
        x3=t2;
} while(--r);
MUL(x1,*Z++);
*out++=x1;
*out++=x3+*Z++;
*out++=x2+*Z++;
MUL(x4,*Z);
*out=x4;
return;
}

