#include "juicer.h"
#include <math.h>

/* Soft MCFS: divide by expected^alpha. alpha=1 fully flattens the diagonal;
   0.5 keeps near-diagonal brightness while still dampening distance decay. */
#define MCFS_Expected_Alpha 0.1

static
u64
JuicerBlockSum(u16 **image, u32 row, u32 col, u32 bin)
{
    u64 sum = 0;
    u32 yEnd = row + bin;
    u32 xEnd = col + bin;

    for (u32 y = row; y < yEnd; ++y)
    {
        for (u32 x = col; x < xEnd; ++x)
        {
            if (x < y) continue;
            sum += image[y][x - y];
        }
    }

    return sum;
}

void
juicer_find_max_sum(void *in)
{
    juicer_sum_task *task = (juicer_sum_task *)in;
    task->maxSum = 0;
    for (u32 row = task->rowStart; row < task->rowEnd; ++row)
    {
        for (u32 dist = 0; dist < task->pixels - row; ++dist)
        {
            u64 sum = JuicerBlockSum(task->sourceImage, row * 2, (row + dist) * 2, 2);
            if (sum > task->maxSum) task->maxSum = sum;
        }
    }
}

void
juicer_sum_level(void *in)
{
    juicer_sum_task *task = (juicer_sum_task *)in;
    for (u32 row = task->rowStart; row < task->rowEnd; ++row)
    {
        for (u32 dist = 0; dist < task->pixels - row; ++dist)
        {
            u64 sum = JuicerBlockSum(task->sourceImage, row * 2, (row + dist) * 2, 2);
            if (!sum)
            {
                task->image[row][dist] = 0;
                continue;
            }

            u32 scaled = (u32)((f64)sum * task->scale + 0.5);
            if (!scaled) scaled = 1;
            if (scaled > u16_max) scaled = u16_max;
            task->image[row][dist] = (u16)scaled;
        }
    }
}

/* Partition around pivot; returns final pivot index. */
static
u32
JuicerPartition(u16 *a, u32 left, u32 right, u32 pivotIndex)
{
    u16 pivot = a[pivotIndex];
    u16 tmp = a[pivotIndex];
    a[pivotIndex] = a[right];
    a[right] = tmp;

    u32 store = left;
    for (u32 i = left; i < right; ++i)
    {
        if (a[i] < pivot)
        {
            tmp = a[store];
            a[store] = a[i];
            a[i] = tmp;
            ++store;
        }
    }

    tmp = a[store];
    a[store] = a[right];
    a[right] = tmp;
    return store;
}

/* Select the k-th smallest element (0-based) in a[left..right], mutating a. */
static
u16
JuicerQuickSelect(u16 *a, u32 left, u32 right, u32 k)
{
    for (;;)
    {
        if (left == right) return a[left];

        u32 pivotIndex = left + (right - left) / 2;
        pivotIndex = JuicerPartition(a, left, right, pivotIndex);

        if (k == pivotIndex) return a[k];
        if (k < pivotIndex) right = pivotIndex - 1;
        else left = pivotIndex + 1;
    }
}

/* Median of n values in scratch. For even n, averages the two central values
   (numpy / gcMapExplorer behaviour). Mutates scratch. */
static
f64
JuicerMedian(u16 *scratch, u32 n)
{
    if (!n) return 0.0;
    if (n == 1) return (f64)scratch[0];

    if (n & 1)
    {
        return (f64)JuicerQuickSelect(scratch, 0, n - 1, n / 2);
    }

    u16 upper = JuicerQuickSelect(scratch, 0, n - 1, n / 2);
    u16 lower = scratch[0];
    for (u32 i = 1; i < n / 2; ++i)
    {
        if (scratch[i] > lower) lower = scratch[i];
    }
    return 0.5 * ((f64)lower + (f64)upper);
}

static
void
JuicerMapRatioToByte(u16 **image, u32 pixels, f64 *expected, f64 alpha)
{
    for (u32 row = 0; row < pixels; ++row)
    {
        for (u32 dist = 0; dist < pixels - row; ++dist)
        {
            u16 count = image[row][dist];
            if (!count || expected[dist] <= 0.0)
            {
                image[row][dist] = 0;
                continue;
            }

            f64 denom = (alpha >= 0.999) ? expected[dist] : pow(expected[dist], alpha);
            if (denom <= 0.0)
            {
                image[row][dist] = 0;
                continue;
            }

            f64 ratio = (f64)count / denom;
            f64 unit = (log(ratio) / log(2.0) + 2.0) * 0.25;
            if (unit < 0.0) unit = 0.0;
            if (unit > 1.0) unit = 1.0;
            image[row][dist] = (u16)(1.0 + unit * 254.0 + 0.5);
        }
    }
}

void
juicer_observed_expected(void *in)
{
    juicer_oe_task *task = (juicer_oe_task *)in;
    u16 **image = task->image;
    u32 pixels = task->pixels;
    f64 *expected = (f64 *)calloc(pixels, sizeof(f64));
    if (!expected)
    {
        fprintf(stderr, "Out of memory while calculating observed/expected values\n");
        exit(EXIT_FAILURE);
    }

    f64 alpha = 1.0;
    if (task->useMedian)
    {
        /* MCFS: occupancy-weighted median per distance, then soft divide by
           expected^alpha so the near-diagonal stays visible on Pretext maps. */
        u16 *scratch = (u16 *)malloc(pixels * sizeof(u16));
        if (!scratch)
        {
            free(expected);
            fprintf(stderr, "Out of memory while calculating MCFS medians\n");
            exit(EXIT_FAILURE);
        }

        for (u32 dist = 0; dist < pixels; ++dist)
        {
            u32 diagLen = pixels - dist;
            u32 count = 0;
            for (u32 row = 0; row < diagLen; ++row)
            {
                u16 value = image[row][dist];
                if (value) scratch[count++] = value;
            }
            if (!count)
            {
                expected[dist] = 0.0;
                continue;
            }
            f64 occupancy = (f64)count / (f64)diagLen;
            expected[dist] = JuicerMedian(scratch, count) * occupancy;
        }

        free(scratch);
        alpha = MCFS_Expected_Alpha;
    }
    else
    {
        /* Juicebox-style mean expected, including zero bins in the average. */
        for (u32 row = 0; row < pixels; ++row)
        {
            for (u32 dist = 0; dist < pixels - row; ++dist)
            {
                expected[dist] += (f64)image[row][dist];
            }
        }

        for (u32 dist = 0; dist < pixels; ++dist)
        {
            expected[dist] /= (f64)(pixels - dist);
        }
    }

    JuicerMapRatioToByte(image, pixels, expected, alpha);
    free(expected);
}
