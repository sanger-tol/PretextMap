/*
Copyright (c) Wellcome Sanger Institute, 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*/

#include "lod_sum.h"

static
u64
LodBlockSum(u16 **image, u32 row, u32 col, u32 bin)
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
lod_find_max_sum(void *in)
{
    lod_sum_task *task = (lod_sum_task *)in;
    task->maxSum = 0;
    for (u32 row = task->rowStart; row < task->rowEnd; ++row)
    {
        for (u32 dist = 0; dist < task->pixels - row; ++dist)
        {
            u64 sum = LodBlockSum(task->sourceImage, row * 2, (row + dist) * 2, 2);
            if (sum > task->maxSum) task->maxSum = sum;
        }
    }
}

void
lod_sum_level(void *in)
{
    lod_sum_task *task = (lod_sum_task *)in;
    for (u32 row = task->rowStart; row < task->rowEnd; ++row)
    {
        for (u32 dist = 0; dist < task->pixels - row; ++dist)
        {
            u64 sum = LodBlockSum(task->sourceImage, row * 2, (row + dist) * 2, 2);
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
