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

#ifndef MAX_NORM_H
#define MAX_NORM_H

#include "Header.h"

struct max_norm_task
{
    u16 **finestImage;
    u16 **image;
    u32 finestPixels;
    u32 level;
    u32 numberOfLevels;
};

void max_norm(void *in);

#endif
