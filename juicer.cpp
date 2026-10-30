#include "juicer.h"
#include <math.h>

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

void
juicer_observed_expected(void *in)
{
    juicer_oe_task *task = (juicer_oe_task *)in;
    u16 **image = task->image;
    u32 pixels = task->pixels;
    f64 *expected = (f64 *)calloc(pixels, sizeof(f64));
    if (!expected)
    {
        fprintf(stderr, "Out of memory while calculating Juicer observed/expected values\n");
        exit(EXIT_FAILURE);
    }

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

            f64 ratio = (f64)count / expected[dist];
            f64 unit = (log(ratio) / log(2.0) + 2.0) * 0.25;
            if (unit < 0.0) unit = 0.0;
            if (unit > 1.0) unit = 1.0;
            image[row][dist] = (u16)(1.0 + unit * 254.0 + 0.5);
        }
    }

    free(expected);
}
