#ifndef JUICER_H
#define JUICER_H

#include "Header.h"

enum juicer_mode
{
    Juicer_None = 0,
    Juicer_OE = 1,
    Juicer_Balance = 2,
    Juicer_MCFS = 3
};

struct juicer_sum_task
{
    u16 **sourceImage;
    u16 **image;
    u32 pixels;
    u32 rowStart;
    u32 rowEnd;
    u64 maxSum;
    f64 scale;
};

struct juicer_oe_task
{
    u16 **image;
    u32 pixels;
    u08 useMedian; /* 0: mean over all bins (Juicebox-style); 1: MCFS median of non-zeros */
};

void juicer_find_max_sum(void *in);
void juicer_sum_level(void *in);
void juicer_observed_expected(void *in);

#endif
