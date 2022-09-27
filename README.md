# uc_resample

Hacked together from JOS [resample-1.8.1](https://ccrma.stanford.edu/~jos/resample/Free_Resampling_Software.html) to run on basic processors with no FPU.
Optimised for block-based dsp, the function generates a fixed number of output samples and determines number of input samples from the rescaling factor.
Overall effect is a speedy repitch algorithm.

Not currently stable/working
