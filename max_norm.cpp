#include "max_norm.h"
#include <math.h>

#define Max_Norm_Histogram_Bins 4096
#define Max_Norm_Fine_Low_Clip 0.08
#define Max_Norm_Fine_High_Clip 0.005
#define Max_Norm_Fine_Gamma 0.8

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
u32
MaxNormHistogramBin(f64 value, f64 low, f64 span)
{
    if (span <= 1e-12) return 0;

    f64 unit = (value - low) / span;
    if (unit < 0.0) unit = 0.0;
    if (unit > 1.0) unit = 1.0;
    return (u32)(unit * (Max_Norm_Histogram_Bins - 1));
}

static
u16
MaxNormByte(u64 count, f64 low, f64 high, f64 gamma)
{
    if (!count || log((f64)count) < low) return 0;
    if (high <= low) return 255;

    f64 unit = (log((f64)count) - low) / (high - low);
    if (unit > 1.0) unit = 1.0;
    f64 mapped = pow(unit, gamma) * 255.0;
    u32 byte = (u32)(mapped + 0.5);
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
    u64 nonzero = 0;

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
            ++nonzero;
            seen = 1;
        }
    }

    if (!seen) return;

    f64 fineFraction = task->numberOfLevels > 1
        ? 1.0 - (f64)task->level / (f64)(task->numberOfLevels - 1)
        : 1.0;
    f64 lowClip = Max_Norm_Fine_Low_Clip * fineFraction;
    f64 highClip = Max_Norm_Fine_High_Clip * fineFraction;
    f64 gamma = 1.0 + (Max_Norm_Fine_Gamma - 1.0) * fineFraction;
    f64 logSpan = maxLog - minLog;
    u64 histogram[Max_Norm_Histogram_Bins] = {};
    for (u32 row = 0; row < nPixels; ++row)
    {
        u32 width = nPixels - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            u64 sum = MaxNormBlockSum(finestImage, row * bin, (row + dist) * bin, bin);
            if (!sum) continue;
            ++histogram[MaxNormHistogramBin(log((f64)sum), minLog, logSpan)];
        }
    }

    u64 lowTarget = (u64)((f64)nonzero * lowClip);
    u64 highTarget = (u64)((f64)nonzero * (1.0 - highClip));
    u64 cumulative = 0;
    u32 lowBin = 0;
    u32 highBin = Max_Norm_Histogram_Bins - 1;
    u08 lowFound = 0;
    for (u32 index = 0; index < Max_Norm_Histogram_Bins; ++index)
    {
        cumulative += histogram[index];
        if (!lowFound && cumulative > lowTarget)
        {
            lowBin = index;
            lowFound = 1;
        }
        if (cumulative >= highTarget)
        {
            highBin = index;
            break;
        }
    }

    f64 lowLog = minLog + logSpan * (f64)lowBin / (f64)(Max_Norm_Histogram_Bins - 1);
    f64 highLog = minLog + logSpan * (f64)highBin / (f64)(Max_Norm_Histogram_Bins - 1);
    if (highLog <= lowLog)
    {
        lowLog = minLog;
        highLog = maxLog;
    }

    for (u32 row = 0; row < nPixels; ++row)
    {
        u32 width = nPixels - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            u64 sum = MaxNormBlockSum(finestImage, row * bin, (row + dist) * bin, bin);
            image[row][dist] = MaxNormByte(sum, lowLog, highLog, gamma);
        }
    }
}
