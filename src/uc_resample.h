/*
 * FILE: resample.h
 *
 * The configuration constants below govern
 * the number of bits in the input sample and filter coefficients, the 
 * number of bits to the right of the binary-point for fixed-point math, etc.
 *
 */

#include "stdefs.h"
#include <stdio.h>

/* Conversion constants */
#define Nhc       8
#define Na        7
#define Np       (Nhc+Na)
#define Npc      (1<<Nhc)
#define Amask    ((1<<Na)-1)
#define Pmask    ((1<<Np)-1)
#define Nh       16
#define Nb       16
#define Nhxn     14
#define Nhg      (Nh-Nhxn)
#define NLpScl   13

/* Description of constants:
 *
 * Npc - is the number of look-up values available for the lowpass filter
 *    between the beginning of its impulse response and the "cutoff time"
 *    of the filter.  The cutoff time is defined as the reciprocal of the
 *    lowpass-filter cut off frequence in Hz.  For example, if the
 *    lowpass filter were a sinc function, Npc would be the index of the
 *    impulse-response lookup-table corresponding to the first zero-
 *    crossing of the sinc function.  (The inverse first zero-crossing
 *    time of a sinc function equals its nominal cutoff frequency in Hz.)
 *    Npc must be a power of 2 due to the details of the current
 *    implementation. The default value of 512 is sufficiently high that
 *    using linear interpolation to fill in between the table entries
 *    gives approximately 16-bit accuracy in filter coefficients.
 *
 * Nhc - is log base 2 of Npc.
 *
 * Na - is the number of bits devoted to linear interpolation of the
 *    filter coefficients.
 *
 * Np - is Na + Nhc, the number of bits to the right of the binary point
 *    in the integer "time" variable. To the left of the point, it indexes
 *    the input array (X), and to the right, it is interpreted as a number
 *    between 0 and 1 sample of the input X.  Np must be less than 16 in
 *    this implementation.
 *
 * Nh - is the number of bits in the filter coefficients. The sum of Nh and
 *    the number of bits in the input data (typically 16) cannot exceed 32.
 *    Thus Nh should be 16.  The largest filter coefficient should nearly
 *    fill 16 bits (32767).
 *
 * Nb - is the number of bits in the input data. The sum of Nb and Nh cannot
 *    exceed 32.
 *
 * Nhxn - is the number of bits to right shift after multiplying each input
 *    sample times a filter coefficient. It can be as great as Nh and as
 *    small as 0. Nhxn = Nh-2 gives 2 guard bits in the multiply-add
 *    accumulation.  If Nhxn=0, the accumulation will soon overflow 32 bits.
 *
 * Nhg - is the number of guard bits in mpy-add accumulation (equal to Nh-Nhxn)
 *
 * NLpScl - is the number of bits allocated to the unity-gain normalization
 *    factor.  The output of the lowpass filter is multiplied by LpScl and
 *    then right-shifted NLpScl bits. To avoid overflow, we must have 
 *    Nb+Nhg+NLpScl < 32.
 */

static INLINE HWORD WordToHword(WORD v, int scl)
{
    HWORD out;
    WORD llsb = (1<<(scl-1));
    v += llsb;          /* round */
    v >>= scl;
    if (v>MAX_HWORD) {
        v = MAX_HWORD;
    } else if (v < MIN_HWORD) {
        v = MIN_HWORD;
    }   
    out = (HWORD) v;
    return out;
}

/* Sampling rate conversion using linear interpolation for maximum speed.
 */
static int 
  SrcLinear(HWORD X[], HWORD Y[], double factor, UWORD *Time, UHWORD Nx)
{
    HWORD iconst;
    HWORD *Xp, *Ystart;
    WORD v,x1,x2;
    
    double dt;                  /* Step through input signal */ 
    UWORD dtb;                  /* Fixed-point version of Dt */
    UWORD endTime;              /* When Time reaches EndTime, return to user */
    
    dt = 1.0/factor;            /* Output sampling period */
    dtb = dt*(1<<Np) + 0.5;     /* Fixed-point representation */
    
    Ystart = Y;
    endTime = *Time + (1<<Np)*(WORD)Nx;
    while (*Time < endTime)
    {
        iconst = (*Time) & Pmask;
        Xp = &X[(*Time)>>Np];      /* Ptr to current input sample */
        x1 = *Xp++;
        x2 = *Xp;
        x1 *= ((1<<Np)-iconst);
        x2 *= iconst;
        v = x1 + x2;
        *Y++ = WordToHword(v,Np);   /* Deposit output */
        *Time += dtb;               /* Move to next sample by time increment */
    }
    return (Y - Ystart);            /* Return number of output samples */
}

typedef struct resampler {
    UWORD Time;     /* Current-time pointer for converter */
    UWORD Xp;       /* Current "now"-sample pointer for input */
    UWORD Xread;    /* Position in input array to read into */
    UWORD Xoff;     /* Input offset */
    UHWORD Nx;      /* Number of samples to process each iteration */
    UWORD last;     /* Have we read the last samples yet? */
    UHWORD Ncreep;  /* Time accumulation */
    UWORD OBUFFSIZE;/* Output buffer size (block size) */
    UWORD IBUFFSIZE;/* Input buffer size */
    int xIndex;          /* Input buffer index */
} resample_t;

static int resampleInit(
    resample_t* resampler,
    UWORD obufsize,
    double factor
) {
    resampler->Xoff = 10;
    resampler->OBUFFSIZE = obufsize;
    resampler->IBUFFSIZE = (int)(((double)resampler->OBUFFSIZE)/factor)+ 2*resampler->Xoff;
    resampler->Nx = resampler->IBUFFSIZE - 2*resampler->Xoff;     /* # of samples to process each iteration */
    resampler->last = 0;                   /* Have not read last input sample yet */
    resampler->Xp = resampler->Xoff;                  /* Current "now"-sample pointer for input */
    resampler->Xread = resampler->Xoff;               /* Position in input array to read into */
    resampler->Time = (resampler->Xoff<<Np);          /* Current-time pointer for converter */
    #ifdef PRINTDEBUGINFO
        printf("Output buffer size: %d\n", resampler->OBUFFSIZE);
        printf("Input buffer size: %d\n", resampler->IBUFFSIZE);
        printf("Input offset size: %d\n", resampler->Xoff);
        printf("Input samples to process per block: %d\n", resampler->Nx);
        printf("Last samples read?: %d\n", resampler->last);
        printf("Current input array index: %d\n", resampler->Xp);
        printf("Position in input array to read into: %d\n", resampler->Xread);
        printf("Current Time pointer: %d\n", (resampler->Time>>Np));
    #endif
}

static int resampleFast(
    resample_t* resampler,     /* Struct to hold resampler internals between calls */
    HWORD* input,               /* Pointer to input samples */
    HWORD* output,              /* Pointer to output samples */
    double factor               /* Resampler factor */
) {
    UHWORD Nout = SrcLinear(input, output, factor, &resampler->Time, resampler->Nx);
    #ifdef PRINTDEBUGINFO
        printf("Nx: %d, Nout: %d\n", resampler->Nx, Nout);
    #endif
    resampler->Time -= (resampler->Nx<<Np);       /* Move converter Nx samples back in time */
    resampler->Xp += resampler->Nx;               /* Advance by number of samples processed */
    resampler->Ncreep = (resampler->Time>>Np) - resampler->Xoff; /* Calc time accumulation in Time */
    if (resampler->Ncreep) {
        resampler->Time -= (resampler->Ncreep<<Np);    /* Remove time accumulation */
        resampler->Xp += resampler->Ncreep;            /* and add it to read pointer */
    }
    for (resampler->xIndex=0; resampler->xIndex<(resampler->IBUFFSIZE-resampler->Xp+resampler->Xoff); resampler->xIndex++) { /* Copy part of input signal */
        input[resampler->xIndex] = input[resampler->xIndex+resampler->Xp-resampler->Xoff]; /* that must be re-used */
    }
    resampler->Xread = resampler->xIndex;              /* Pos in input buff to read new data into */
    resampler->Xp = resampler->Xoff;
    return Nout;
}