#include "max_norm.h"
#include <math.h>

static
u64
MaxNormBlockSum(u16 **image, u32 row, u32 col, u32 bin)
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
MaxNormByte(u64 count, f64 low, f64 span)
{
    if (!count) return 0;

    f64 unit = span > 1e-12
        ? (log((f64)count) - low) / span
        : 1.0;
    if (unit < 0.0) unit = 0.0;
    if (unit > 1.0) unit = 1.0;
    u32 byte = (u32)(unit * 255.0 + 0.5);
    if (!byte) byte = 1;
    if (byte > 255) byte = 255;
    return (u16)byte;
}

void
max_norm(void *in)
{
    max_norm_task *task = (max_norm_task *)in;
    u32 bin = 1u << task->level;
    u32 nPixels = task->finestPixels >> task->level;
    u16 **finestImage = task->finestImage;
    u16 **image = task->image;
    f64 minLog = 0.0;
    f64 maxLog = 0.0;
    u08 seen = 0;

    for (u32 row = 0; row < nPixels; ++row)
    {
        u32 width = nPixels - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            u64 sum = MaxNormBlockSum(finestImage, row * bin, (row + dist) * bin, bin);
            if (!sum) continue;

            f64 value = log((f64)sum);
            if (!seen || value < minLog) minLog = value;
            if (value > maxLog) maxLog = value;
            seen = 1;
        }
    }

    if (!seen) return;

    f64 logSpan = maxLog - minLog;
    for (u32 row = 0; row < nPixels; ++row)
    {
        u32 width = nPixels - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            u64 sum = MaxNormBlockSum(finestImage, row * bin, (row + dist) * bin, bin);
            image[row][dist] = MaxNormByte(sum, minLog, logSpan);
        }
    }
}
