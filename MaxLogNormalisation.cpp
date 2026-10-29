#include "MaxLogNormalisation.h"
#include <math.h>

static
u64
LogBlockSum(u16 **image, u32 row, u32 col, u32 bin)
{
    u64 sum = 0;
    u32 yEnd = row + bin;
    u32 xEnd = col + bin;

    for (u32 y = row; y < yEnd; ++y)
    {
        for (u32 x = col; x < xEnd; ++x)
        {
            if (x < y) continue;
            u32 min = x < y ? x : y;
            u32 max = x < y ? y : x;
            sum += image[min][max - min];
        }
    }

    return sum;
}

static
u16
LogByte(u64 count, f64 low, f64 span)
{
    if (!count) return 0;

    u32 byte = 255;
    if (span > 1e-12)
    {
        f64 unit = (log((f64)count) - log(low)) / span;
        if (unit < 0.0) unit = 0.0;
        if (unit > 1.0) unit = 1.0;
        byte = (u32)(unit * 255.0 + 0.5);
    }

    if (byte < 1) byte = 1;
    if (byte > 255) byte = 255;
    return (u16)byte;
}

void
MaxLogNormalisation(void *in)
{
    max_log_normalisation_task *task = (max_log_normalisation_task *)in;
    u32 level = task->level;
    u32 bin = 1u << level;
    u32 nPixels = task->finestPixels >> level;
    u16 **finestImage = task->finestImage;
    u16 **image = task->image;
    f64 low = 0.0;
    f64 high = 0.0;
    u08 seen = 0;

    for (u32 row = 0; row < nPixels; ++row)
    {
        u32 width = nPixels - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            u64 sum = LogBlockSum(finestImage, row * bin, (row + dist) * bin, bin);
            if (!sum) continue;

            f64 value = (f64)sum;
            if (!seen || value < low) low = value;
            if (value > high) high = value;
            seen = 1;
        }
    }

    if (!seen || high <= 0.0) return;

    f64 span = log(high) - log(low);
    for (u32 row = 0; row < nPixels; ++row)
    {
        u32 width = nPixels - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            u64 sum = LogBlockSum(finestImage, row * bin, (row + dist) * bin, bin);
            image[row][dist] = LogByte(sum, low, span);
        }
    }
}
