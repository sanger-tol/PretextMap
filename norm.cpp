#include "norm.h"
#include <math.h>

u32 Norm_Mode = Norm_KL;

#define KL_Norm_Range (1 << 15)
#define KL_Norm_Tol (1e-4)
#define KL_Max_Iter 128

const char *
NormModeName(void)
{
    if (Norm_Mode == Norm_VCSqrt) return "vcsqrt";
    return "kl";
}

/* image[row][distance] = contact between row and row+distance (upper triangle). */
static
void
SymvUpper(u16 **image, u32 n, f64 *right, f64 *out)
{
    memset(out, 0, (size_t)n * sizeof(f64));

    for (u32 row = 0; row < n; ++row)
    {
        u16 *cells = image[row];
        f64 left = right[row];
        u32 width = n - row;

        for (u32 dist = 0; dist < width; ++dist)
        {
            f64 count = (f64)cells[dist];
            u32 col = row + dist;

            out[row] += count * right[col];
            if (dist) out[col] += count * left;
        }
    }
}

static
void
ApplyBalanceWeights(u16 **image, u32 n, f64 *weight)
{
    f64 peak = 0.0;

    for (u32 row = 0; row < n; ++row)
    {
        u32 width = n - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            f64 balanced = (f64)image[row][dist] * weight[row] * weight[row + dist];
            if (balanced > peak) peak = balanced;
        }
    }

    if (peak <= 0.0) return;

    f64 scale = (f64)(1 << 15) / peak;
    for (u32 row = 0; row < n; ++row)
    {
        u32 width = n - row;
        for (u32 dist = 0; dist < width; ++dist)
        {
            f64 balanced = (f64)image[row][dist] * weight[row] * weight[row + dist] * scale;
            if (balanced < 0.0) balanced = 0.0;
            if (balanced > 65535.0) balanced = 65535.0;
            image[row][dist] = (u16)(balanced + 0.5);
        }
    }
}

static
void
NormVCSqrt(u16 **image, u32 n)
{
    f64 *weight = (f64 *)malloc(3 * (size_t)n * sizeof(f64));
    if (!weight)
    {
        fprintf(stderr, "Out of memory while applying vcsqrt normalisation\n");
        exit(EXIT_FAILURE);
    }
    f64 *ones = weight + n;
    f64 *coverage = ones + n;

    for (u32 bin = 0; bin < n; ++bin) ones[bin] = 1.0;
    SymvUpper(image, n, ones, coverage);

    for (u32 bin = 0; bin < n; ++bin)
    {
        if (coverage[bin] > 0.0) weight[bin] = 1.0 / sqrt(coverage[bin]);
        else weight[bin] = 0.0;
    }

    ApplyBalanceWeights(image, n, weight);
    free(weight);
}

static
void
NormKL(u16 **image, u32 n, f32 *scalars)
{
    u16 max;
    f32 maxNorm;
    u32 klIter = 0;

    do
    {
        ForLoop(n)
        {
            max = 0;

            ForLoop2(n)
            {
                u32 idMin = Min(index, index2);
                u32 idMax = Max(index, index2);
                u16 pixel = image[idMin][idMax - idMin];

                max = Max(max, pixel);
            }

            scalars[index] = max ? 1.0f / sqrtf((f32)max) : 1.0f;
        }

        ForLoop(n)
        {
            ForLoop2(n - index)
            {
                image[index][index2] = (u16)((f32)image[index][index2] * scalars[index] * scalars[index + index2] * (f32)KL_Norm_Range);
            }
        }

        maxNorm = 0.0f;
        ForLoop(n)
        {
            max = 0;

            ForLoop2(n)
            {
                u32 idMin = Min(index, index2);
                u32 idMax = Max(index, index2);
                u16 pixel = image[idMin][idMax - idMin];

                max = Max(max, pixel);
            }

            f32 tmp = max ? ((f32)KL_Norm_Range - (f32)max) / (f32)KL_Norm_Range : 0.0f;
            tmp = tmp > 0.0f ? tmp : -tmp;
            maxNorm = Max(maxNorm, tmp);
        }
    } while (maxNorm > (f32)KL_Norm_Tol && klIter++ < KL_Max_Iter);
}

void
ApplyNorm(u16 **image, u32 n, f32 *scalars, u32 mode)
{
    if (mode == Norm_VCSqrt) NormVCSqrt(image, n);
    else NormKL(image, n, scalars);
}
