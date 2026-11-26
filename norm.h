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

#ifndef NORM_H
#define NORM_H

#include "Header.h"

#define Norm_KL 0
#define Norm_VCSqrt 1

extern u32 Norm_Mode;

struct contrast_task
{
    u32 lodIndex;
    u32 normMode;
};

const char *NormModeName(void);
void ApplyNorm(u16 **image, u32 n, f32 *scalars, u32 mode);

#endif
