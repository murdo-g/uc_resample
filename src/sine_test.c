#define PRINTDEBUGINFO 1

#include "uc_resample.h"

#define BLOCKSIZE 128
#define ORIGINAL_SR 48000
#define SAMPLE_DUR_S 10
#define RESAMPLE_FACTOR 1.01

int readData(FILE* inputfile, HWORD* inputBuf, int arraySize, int Xoff) {
    int    i, Nsamps, nret;
    Nsamps = arraySize - Xoff;   /* Calculate number of samples to get */
    inputBuf += Xoff;                 /* Start at designated sample number */
    nret = fread(inputBuf, sizeof(HWORD), Nsamps, inputfile);
    if (feof(inputfile)) { printf("EOF reached!\n"); return nret; }
    else {return 0;}
}

int main() {
    FILE *input;
    input = fopen("../sine440.raw", "r");
    FILE *output;
    output = fopen("../sine444.raw", "w");
    printf("=========================\nuc_resample Test.\n\n");

    resample_t resample;
    resampleInit(&resample, BLOCKSIZE, RESAMPLE_FACTOR);
    HWORD inSamples[resample.IBUFFSIZE];
    HWORD outSamples[resample.OBUFFSIZE];

    for (int i=0; i<resample.Xoff; inSamples[i++]=0); /* Need Xoff zeros at begining of sample */
    
    for(int i=0; i<1875; i++) {
        readData(input, inSamples, resample.IBUFFSIZE, resample.Xread);
        UHWORD Nout = resampleFast(&resample, inSamples, outSamples, RESAMPLE_FACTOR);
        fwrite(outSamples, sizeof(HWORD), Nout, output);
    }
    fclose(input);
    fclose(output);
    
    return 0;
}