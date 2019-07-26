/*
Copyright (c) 2019 Ed Harry, Wellcome Sanger Institute

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define PretextMap_Version "PretextMap Version 0.0.41"

#include "Header.h"
#include <math.h>

global_variable
u08
Licence[] = R"lic(
Copyright (c) 2019 Ed Harry, Wellcome Sanger Institute

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)lic";

global_variable
u08
ThirdParty[] = R"thirdparty(
Third-party software and resources used in this project, along with their respective licenses:

    This project uses libdeflate (https://github.com/ebiggers/libdeflate)
        
        Copyright 2016 Eric Biggers

        Permission is hereby granted, free of charge, to any person
        obtaining a copy of this software and associated documentation files
        (the "Software"), to deal in the Software without restriction,
        including without limitation the rights to use, copy, modify, merge,
        publish, distribute, sublicense, and/or sell copies of the Software,
        and to permit persons to whom the Software is furnished to do so,
        subject to the following conditions:

        The above copyright notice and this permission notice shall be
        included in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
        EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
        MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
        NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
        BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
        ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.


    This project uses stb_sprintf (https://github.com/nothings/stb/blob/master/stb_sprintf.h)

        ALTERNATIVE B - Public Domain (www.unlicense.org)
        This is free and unencumbered software released into the public domain.
        Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
        software, either in source code form or as a compiled binary, for any purpose, 
        commercial or non-commercial, and by any means.
        In jurisdictions that recognize copyright laws, the author or authors of this 
        software dedicate any and all copyright interest in the software to the public 
        domain. We make this dedication for the benefit of the public at large and to 
        the detriment of our heirs and successors. We intend this dedication to be an 
        overt act of relinquishment in perpetuity of all present and future rights to 
        this software under copyright law.
        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
        AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
        ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
        WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


    This project uses stb_dxt (https://github.com/nothings/stb/blob/master/stb_dxt.h)

        ALTERNATIVE B - Public Domain (www.unlicense.org)
        This is free and unencumbered software released into the public domain.
        Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
        software, either in source code form or as a compiled binary, for any purpose, 
        commercial or non-commercial, and by any means.
        In jurisdictions that recognize copyright laws, the author or authors of this 
        software dedicate any and all copyright interest in the software to the public 
        domain. We make this dedication for the benefit of the public at large and to 
        the detriment of our heirs and successors. We intend this dedication to be an 
        overt act of relinquishment in perpetuity of all present and future rights to 
        this software under copyright law.
        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
        AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
        ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
        WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
)thirdparty";

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconditional-uninitialized"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#pragma clang diagnostic pop

global_variable
memory_arena
Working_Set;

global_variable
thread_pool *
Thread_Pool;

#include "LineBufferQueue.cpp"

global_variable
threadSig
Processing_Body_Thread = 0;

global_variable
u32
Processing_Body = 0;

global_variable
u32
Number_of_Threads;

global_variable
threadSig
Number_of_Contigs = 0;

global_variable
volatile u64
Total_Genome_Length = 0;

global_variable
threadSig
Number_of_Header_Lines = 0;

global_function
void
IncramentNumberOfHeaderLines()
{
    __sync_fetch_and_add(&Number_of_Header_Lines, 1);
}

global_function
void
DecrementNumberHeaderLines()
{
    __sync_fetch_and_sub(&Number_of_Header_Lines, 1);
}

struct
contig_preprocess_node
{
    u32 length;
    u32 hashedName;
    u32 name[16];
    contig_preprocess_node *next;
};

struct
contig
{
    u32 length;
    u32 pad;
    u64 previousCumulativeLength;
    u32 name[16];
};

#define Contig_Hash_Table_Size 2609
#define Contig_Hash_Table_Seed 6697139165994551568

global_function
u08 *
PushStringIntoIntArray(u32 *intArray, u32 arrayLength, u08 *string, u08 stringTerminator = '\0')
{
    u08 *stringToInt = (u08 *)intArray;
    u32 stringLength = 0;

    while(*string != stringTerminator)
    {
        *(stringToInt++) = *(string++);
        ++stringLength;
    }

    while (stringLength & 3)
    {
        *(stringToInt++) = 0;
        ++stringLength;
    }

    for (   u32 index = (stringLength >> 2);
            index < arrayLength;
            ++index )
    {
        intArray[index] = 0;
    }

    return(string);
}

global_function
u32
GetHashedContigName(u32 *name, u32 nInts)
{
    return(FastHash32((void *)name, (u64)(nInts * 4), Contig_Hash_Table_Seed) % Contig_Hash_Table_Size);
}

struct
contig_hash_table_node
{
    u32 index;
    u32 pad;
    contig_hash_table_node *next;
};

global_variable
contig *
Contigs = 0;

global_variable
contig_hash_table_node **
Contig_Hash_Table = 0;

global_function
void
InitiateContigHashTable()
{
    ForLoop(Contig_Hash_Table_Size)
    {
        *(Contig_Hash_Table + index) = 0;
    }
}

global_function
void
InsertContigIntoHashTable(u32 index, u32 hash)
{
    contig_hash_table_node *node = Contig_Hash_Table[hash];
    contig_hash_table_node *nextNode = node ? node->next : 0;

    while (nextNode)
    {
        node = nextNode;
        nextNode = node->next;
    }
    
    contig_hash_table_node *newNode = PushStruct(Working_Set, contig_hash_table_node);
    
    newNode->index = index;
    newNode->next = 0;
    
    (node ? node->next : Contig_Hash_Table[hash]) = newNode;
}

global_function
u32
ContigHashTableLookup(u32 *name, u32 nameLength, u32 *index)
{
    u32 result = 1;

    contig_hash_table_node *node = Contig_Hash_Table[GetHashedContigName(name, nameLength)];
    if (node)
    {
        while (!AreNullTerminatedStringsEqual(name, (Contigs + node->index)->name, nameLength))
        {
            if (node->next)
            {
                node = node->next;
            }
            else
            {
                result = 0;
                break;
            }
        }

        if (result)
        {
            *index = node->index;
        }
    }
    else
    {
        result = 0;
    }

    return(result);
}

global_function
u32
ContigHashTableLookup(u32 *name, u32 nameLength, contig **cont)
{
    u32 result = 1;

    contig_hash_table_node *node = Contig_Hash_Table[GetHashedContigName(name, nameLength)];
    if (node)
    {
        while (!AreNullTerminatedStringsEqual(name, (Contigs + node->index)->name, nameLength))
        {
            if (node->next)
            {
                node = node->next;
            }
            else
            {
                result = 0;
                break;
            }
        }

        if (result)
        {
            *cont = Contigs + node->index;
        }
    }
    else
    {
        result = 0;
    }

    return(result);
}

global_function
u32
ContigHashTableLookup(u32 *name, u32 nameLength, u32 *index, contig **cont)
{
    u32 result = 1;

    contig_hash_table_node *node = Contig_Hash_Table[GetHashedContigName(name, nameLength)];
    if (node)
    {
        while (!AreNullTerminatedStringsEqual(name, (Contigs + node->index)->name, nameLength))
        {
            if (node->next)
            {
                node = node->next;
            }
            else
            {
                result = 0;
                break;
            }
        }

        if (result)
        {
            *index = node->index;
            *cont = Contigs + node->index;
        }
    }
    else
    {
        result = 0;
    }

    return(result);
}

global_variable
volatile contig_preprocess_node *
First_Contig_Preprocess_Node = 0;

global_variable
volatile contig_preprocess_node *
Current_Contig_Preprocess_Node = 0;

global_variable
volatile memory_arena_snapshot
Contig_Preprocess_SnapShot;

global_variable
mutex
Working_Set_rwMutex;

global_variable
u32
Min_Map_Quality = 10;

#define Max_Image_Depth 15
#define Min_Image_Depth 10
#define Number_of_LODs (Max_Image_Depth - Min_Image_Depth + 1)
#define Pixel_Resolution(depth) (1 << (depth))

global_variable
volatile u16***
Images;

global_variable
u32**
Image_Histograms;

global_variable
f32**
Image_Norm_Scalars;

global_function
void
InitaliseImages()
{
    Images = PushArray(Working_Set, volatile u16**, Number_of_LODs);
    Image_Histograms = PushArray(Working_Set, u32*, Number_of_LODs);
    Image_Norm_Scalars = PushArray(Working_Set, f32*, Number_of_LODs);

    for (   u32 depth = Max_Image_Depth, imIndex = 0;
            depth >= Min_Image_Depth;
            --depth, ++imIndex )
    {
        Images[imIndex] = PushArray(Working_Set, volatile u16*, Pixel_Resolution(depth));
        
        Image_Histograms[imIndex] = PushArray(Working_Set, u32, 1 << 8);
        memset(Image_Histograms[imIndex], 0, (1 << 8) * sizeof(u32));

        Image_Norm_Scalars[imIndex] = PushArray(Working_Set, f32, Pixel_Resolution(depth));
        memset(Image_Norm_Scalars[imIndex], 0, Pixel_Resolution(depth) * sizeof(f32));

        ForLoop(Pixel_Resolution(depth))
        {
            Images[imIndex][index] = PushArray(Working_Set, volatile u16, Pixel_Resolution(depth) - index);
            ForLoop2(Pixel_Resolution(depth) - index)
            {
                Images[imIndex][index][index2] = 0;
            }
        }
    }
}

global_function
void
AddReadPairToImage(u64 read1, u64 read2)
{
    f64 factor = 1.0 / (f64)Total_Genome_Length;
    u32 pixel1 = (u32)((f64)read1 * factor * (f64)Pixel_Resolution(Max_Image_Depth));
    u32 pixel2 = (u32)((f64)read2 * factor * (f64)Pixel_Resolution(Max_Image_Depth));

    u32 min = Min(pixel1, pixel2);
    u32 max = Max(pixel1, pixel2);
    volatile u16 *pixel = &Images[0][min][max - min];
#ifndef _WIN32
    volatile u16 oldVal = __atomic_fetch_add(pixel, 1, 0);
#else
    volatile u16 oldVal = __atomic_fetch_add((volatile unsigned long *)pixel, 1, 0);
#endif
    if (oldVal == u16_max)
    {
#ifndef _WIN32
        __atomic_store(pixel, &oldVal, 0);
#else
        __atomic_store((volatile unsigned long *)pixel, (volatile unsigned long *)&oldVal, 0);
#endif
    }
}

global_variable
volatile u64
Total_Reads_Processed;

global_variable
volatile u64
Total_Good_Reads;

enum
file_type
{
    sam,
    pairs,
    undet
};

global_variable
file_type
File_Type = undet;

#define StatusPrintEvery 10000

global_function
void
ProcessBodyLine(u08 *line)
{
    u64 nLinesTotal = __atomic_add_fetch(&Total_Reads_Processed, 1, 0);
    
    while (*line++ != '\t') {}
    u32 len = 1;
    u32 flags;

    if (File_Type == sam)
    {
        while (*++line != '\t') ++len;
        flags = StringToInt(line, len);
    }
    else
    {
        --line;
        flags = 0x1;
    }

    if ((flags < 128) && (flags & 0x1) && !(flags & 0x4) && !(flags & 0x8))
    {
        u32 contigName1[16];
        line = PushStringIntoIntArray(contigName1, ArrayCount(contigName1), ++line, '\t');

        len = 0;
        while (*++line != '\t') ++len;
        u32 relPos1 = StringToInt(line, len);

        u32 mapq;
        if (File_Type == sam)
        {
            len = 0;
            while (*++line != '\t') ++len;
            mapq = StringToInt(line, len);
        }
        else
        {
            mapq = Min_Map_Quality;
        }

        if (mapq >= Min_Map_Quality)
        {
            if (File_Type == sam) while (*++line != '\t') {}
            
            u32 sameContig = 0;
            u32 contigName2[16];

            if (*++line == '=')
            {
                sameContig = 1;
                ++line;
            }
            else
            {
                line = PushStringIntoIntArray(contigName2, ArrayCount(contigName2), line, '\t');
            }

            len = 0;
            while (*++line != '\t') ++len;
            u32 relPos2 = StringToInt(line, len);

            contig *cont = 0;
            ContigHashTableLookup(contigName1, ArrayCount(contigName1), &cont);

            if (cont)
            {
                u64 cummLen1 = cont->previousCumulativeLength;
                u64 read1 = cummLen1 + (u64)relPos1;

                u64 cummLen2 = 0;
                if (sameContig)
                {
                    cummLen2 = cummLen1;
                }
                else
                {
                    cont = 0;
                    ContigHashTableLookup(contigName2, ArrayCount(contigName2), &cont);

                    if (cont)
                    {
                        cummLen2 = cont->previousCumulativeLength;
                    }
                }

                if (cont)
                {
                    u64 read2 = cummLen2 + (u64)relPos2;

                    AddReadPairToImage(read1, read2);

                    __atomic_add_fetch(&Total_Good_Reads, 1, 0);
                }
            }
        }
    }

    if ((nLinesTotal % StatusPrintEvery) == 0)
    {
        char buff[64];
        
        if (File_Type == sam)
            stbsp_snprintf(buff, 64, "%$d read pairs processed, %$d good pairs found", nLinesTotal, Total_Good_Reads);
        else
            stbsp_snprintf(buff, 64, "%$d read pairs processed", nLinesTotal);

        printf("\r%s", buff);
        fflush(stdout);
    }
}

global_function
void
ProcessHeaderLine(u08 *line)
{
    u32 buff32[16];
    u32 buffIndex = 0;
    u08 *buff = (u08 *)buff32;

    if (File_Type == sam)
    {
        while (*line != '\t')
        {
            ++line;
        }
        line += 4;
    }
    else
    {
        line += 12;
    }

    while (*line != '\t')
    {
        *buff++ = *line++;
        ++buffIndex;
    }
    while (buffIndex & 3)
    {
        *buff++ = 0;
        ++buffIndex;
    }
    for (   u32 index = (buffIndex >> 2);
            index < ArrayCount(buff32);
            ++index )
    {
        buff32[index] = 0;
    }
    line += (File_Type == sam ? 4 : 1);

    u32 count = 1;
    while (*++line != '\n')
    {
        ++count;
    }
    u32 length = StringToInt(line, count);
    u32 hash = GetHashedContigName(buff32, ArrayCount(buff32));

    LockMutex(Working_Set_rwMutex);

    if (!First_Contig_Preprocess_Node)
    {
        TakeMemoryArenaSnapshot(&Working_Set, (memory_arena_snapshot *)&Contig_Preprocess_SnapShot);
    }

    contig_preprocess_node *node = PushStruct(Working_Set, contig_preprocess_node);
    if (!First_Contig_Preprocess_Node)
    {
        First_Contig_Preprocess_Node = node;
    }
    else
    {
        Current_Contig_Preprocess_Node->next = node;
    }
    
    node->length = length;
    node->hashedName = hash;
    node->next = 0;

    ForLoop(ArrayCount(buff32))
    {
        node->name[index] = buff32[index];
    }

    Current_Contig_Preprocess_Node = node;
    UnlockMutex(Working_Set_rwMutex);
    
    DecrementNumberHeaderLines();
}

enum
sort_by
{
    sortByLength,
    sortByName,
    noSort
};

enum
sort_order
{
    ascend,
    descend
};

global_variable
sort_by
Sort_By = sortByLength;

global_variable
sort_order
Sort_Order = descend;

global_variable
threadSig
Finishing_Header = 0;

global_function
void
FinishProcessingHeader()
{
    if (__atomic_fetch_add(&Finishing_Header, 1, 0) == 0)
    {
        while (Number_of_Header_Lines) {}

        u32 *indexes = PushArray(Working_Set, u32, Number_of_Contigs);
        u32 *tmpSpace = PushArray(Working_Set, u32, Number_of_Contigs);
        contig_preprocess_node **nodes = PushArray(Working_Set, contig_preprocess_node*, Number_of_Contigs);

        u32 tmpIndex = 0;
        TraverseLinkedList((contig_preprocess_node *)First_Contig_Preprocess_Node, contig_preprocess_node)
        {
            nodes[tmpIndex] = node;
            indexes[tmpIndex] = tmpIndex;
            ++tmpIndex;

            Total_Genome_Length += (u64)node->length;
        }

        u32 hist_[512];
        u32 *hist = (u32 *)hist_;
        u32 *nextHist = hist + 256;

        for (   u32 countIndex = 0;
                countIndex < (Sort_By == noSort ? 0 : (Sort_By == sortByLength ? 1 : (ArrayCount(First_Contig_Preprocess_Node->name))));
                ++countIndex )
        {
            ForLoop(255)
            {
                hist[index] = 0;
            }

            ForLoop(Number_of_Contigs)
            {
                u32 val = Sort_By == sortByLength ? (*(nodes+indexes[index]))->length : (*(nodes+indexes[index]))->name[ArrayCount(First_Contig_Preprocess_Node->name) - 1 - countIndex];
                u32 histVal = Sort_Order == descend ? 255 - (val & 0xff) : (val & 0xff);

                ++hist[histVal];
            }

            for (   u32 shift = 0;
                    shift < 24;
                    shift += 8 )
            {
                u32 total = 0;
                ForLoop(255)
                {
                    u32 tmp = hist[index];
                    hist[index] = total;
                    total += tmp;
                    nextHist[index] = 0;
                }
                hist[255] = total;

                ForLoop(Number_of_Contigs)
                {
                    u32 val = Sort_By == sortByLength ? ((*(nodes+indexes[index]))->length >> shift) : ((*(nodes+indexes[index]))->name[ArrayCount(First_Contig_Preprocess_Node->name) - 1 - countIndex] >> shift);
                    u32 histVal = Sort_Order == descend ? 255 - (val & 0xff) : (val & 0xff);
                    u32 nextHistVal = Sort_Order == descend ? 255 - ((val >> 8) & 0xff): ((val >> 8) & 0xff);

                    u32 newIndex = hist[histVal]++;
                    tmpSpace[newIndex] = indexes[index];
                    ++nextHist[nextHistVal];
                }
                u32 *tmp = tmpSpace;
                tmpSpace = indexes;
                indexes = tmp;

                tmp = nextHist;
                nextHist = hist;
                hist = tmp;
            }

            u32 total = 0;
            ForLoop(255)
            {
                u32 tmp = hist[index];
                hist[index] = total;
                total += tmp;
            }
            hist[255] = total;

            ForLoop(Number_of_Contigs)
            {
                u32 val = Sort_By == sortByLength ? ((*(nodes+indexes[index]))->length >> 24) : ((*(nodes+indexes[index]))->name[ArrayCount(First_Contig_Preprocess_Node->name) - 1 - countIndex] >> 24);
                u32 histVal = Sort_Order == descend ? 255 - (val & 0xff) : (val & 0xff);

                u32 newIndex = hist[histVal]++;
                tmpSpace[newIndex] = indexes[index];
            }

            if (Sort_By == sortByLength)
            {
                indexes = tmpSpace;
            }
            else
            {
                u32 *tmp = tmpSpace;
                tmpSpace = indexes;
                indexes = tmp;
            }
        }

        u32 **names = PushArray(Working_Set, u32*, Number_of_Contigs);
        u32 *lengths = PushArray(Working_Set, u32, Number_of_Contigs);
        u32 *hashes = PushArray(Working_Set, u32, Number_of_Contigs);
        ForLoop(Number_of_Contigs)
        {
            contig_preprocess_node *node = nodes[indexes[index]];
            lengths[index] = node->length;
            hashes[index] = node->hashedName;
            names[index] = PushArray(Working_Set, u32, ArrayCount(node->name));
            ForLoop2(ArrayCount(node->name))
            {
                names[index][index2] = node->name[index2];
            }
        }

        RestoreMemoryArenaFromSnapshot(&Working_Set, (memory_arena_snapshot *)&Contig_Preprocess_SnapShot);

        u64 cummLength = 0;
        Contigs = PushArray(Working_Set, contig, Number_of_Contigs);
        ForLoop(Number_of_Contigs)
        {
            u32 length = lengths[index];
            u32 *name = names[index];

            contig *cont = Contigs + index; 
            cont->length = length;
            cont->previousCumulativeLength = cummLength;
            cummLength += cont->length;
            ForLoop2(ArrayCount(cont->name))
            {
                cont->name[index2] = name[index2];
            }
        }

        Contig_Hash_Table = PushArray(Working_Set, contig_hash_table_node*, Contig_Hash_Table_Size);
        u32 *tmpHashes = PushArray(Working_Set, u32, Number_of_Contigs);
        ForLoop(Number_of_Contigs)
        {
            tmpHashes[index] = hashes[index];
        }
        hashes = tmpHashes;

        InitiateContigHashTable();
        ForLoop(Number_of_Contigs)
        {
            InsertContigIntoHashTable(index, hashes[index]);
        }
        FreeLastPush(Working_Set);

        Processing_Body_Thread = 1;

        printf("Mapping to %d contigs, ", Number_of_Contigs);
        if (Sort_By == noSort)
        {
            printf("unsorted");
        }
        else if (Sort_By == sortByLength)
        {
            printf("sorted by length, ");
            if (Sort_Order == descend)
            {
                printf("descending");
            }
            else
            {
                printf("ascending");
            }
        }
        else
        {
            printf("sorted by name, ");
            if (Sort_Order == descend)
            {
                printf("descending");
            }
            else
            {
                printf("ascending");
            }
        }
        if (File_Type == sam)
            printf(". Filtering by minimum mapping quality %d\n", Min_Map_Quality);
        else
            printf("\n");
    }
}

global_function
void
ProcessLine(void *in)
{
    line_buffer *buffer = (line_buffer *)in;
    u08 *line = buffer->line;

    if (Processing_Body)
    {
        ProcessBodyLine(line);
    }
    else if (Processing_Body_Thread)
    {
        Processing_Body = 1;
        ProcessBodyLine(line);
    }
    else if (File_Type == sam && line[0] == '@')
    {
        if (line[1] == 'S')
        {
            ProcessHeaderLine(line);
        }
        else if (line[1] != 'H')
        {
            FinishProcessingHeader();
        }
    }
    else if (File_Type == pairs && line[0] == '#')
    {
        if ((*((u64 *)((void *)line))) == 0x69736d6f72686323) // #chromsi
        {
            ProcessHeaderLine(line);
        }
    }
    else
    {
        FinishProcessingHeader();
        ThreadPoolAddTask(Thread_Pool, ProcessLine, in);
        return;
    }

    AddLineBufferToQueue(Line_Buffer_Queue, buffer);
}

global_function
void
GrabStdIn()
{
    line_buffer *buffer = 0;
    u32 bufferPtr = 0;
    s32 character;
    while ((character = getchar()) != EOF)
    {
        if (!buffer) buffer = TakeLineBufferFromQueue_Wait(Line_Buffer_Queue);
        buffer->line[bufferPtr++] = (u08)character;

        if (bufferPtr == Line_Buffer_Size && character != 10)
        {
            while ((character = getchar()) != 10) {}
            buffer->line[bufferPtr-1] = (u08)character;
        }

        if (character == 10)
        {
            if (Processing_Body)
            {

            }
            else if (Processing_Body_Thread)
            {
                Processing_Body = 1;
            }
            else if (buffer->line[0] == '@' && buffer->line[1] == 'S')
            {
                if (File_Type != sam && File_Type != undet)
                {
                    fprintf(stderr, "Error, inconsistent file type\n");
                    break;
                }
                else if (File_Type == undet)
                {
                    File_Type = sam;
                }

                IncramentNumberOfHeaderLines();
                ++Number_of_Contigs;
            }
            else if ((*((u64 *)buffer->line)) == 0x69736d6f72686323) // #chromsi
            {
                if (File_Type != pairs && File_Type != undet)
                {
                    fprintf(stderr, "Error, inconsistent file type\n");
                    break;
                }
                else if (File_Type == undet)
                {
                    File_Type = pairs;
                }

                IncramentNumberOfHeaderLines();
                ++Number_of_Contigs;
            }

            switch (File_Type)
            {
                case undet:
                    AddLineBufferToQueue(Line_Buffer_Queue, buffer);
                    break;

                case sam:
                case pairs:
                    ThreadPoolAddTask(Thread_Pool, ProcessLine, (void *)buffer);
            }

            buffer = 0;
            bufferPtr = 0;
        }
    }
}

global_function
u16
PixelLookup(u32 x, u32 y)
{
    u32 min = Min(x, y);
    u32 max = Max(x, y);

    return((u16)Images[0][min][max - min]);
}

struct
heap_node
{
    u32 value;
    f32 weight;
};

struct
binary_heap
{
    heap_node *array;
    u32 size;
    u32 pad;
};

struct
weighted_median_kernel
{
    binary_heap *heap;
    f32 **weights;
};

global_function
binary_heap *
InitiateBinaryHeap(memory_arena *arena, u32 capacity)
{
    binary_heap *heap = PushStructP(arena, binary_heap);
    heap->size = 0;
    heap->array = PushArrayP(arena, heap_node, capacity);

    return(heap);
}

global_function
void
ResetBinaryHeap(binary_heap *heap)
{
    heap->size = 0;
}

global_function
u32
HeapParent(u32 i)
{ 
    return((i - 1) / 2);
} 
  
global_function
u32
HeapLeft(u32 i)
{ 
    return(((2 * i) + 1));
} 
  
global_function
u32
HeapRight(u32 i)
{ 
    return(((2 * i) + 2));
} 
  
global_function
heap_node
HeapGetMin(binary_heap *heap)
{ 
    return(heap->array[0]);
} 

global_function
void
HeapInsertKey(binary_heap *heap, heap_node k) 
{ 
    u32 i = heap->size++; 
    heap->array[i] = k; 
  
    while (i != 0 && heap->array[HeapParent(i)].value > heap->array[i].value) 
    { 
        heap_node tmp = heap->array[i];
        heap->array[i] = heap->array[HeapParent(i)];
        heap->array[HeapParent(i)] = tmp;
        i = HeapParent(i); 
    } 
} 

global_function
void
HeapMinHeapify(binary_heap *heap, u32 i) 
{ 
    u32 l = HeapLeft(i); 
    u32 r = HeapRight(i); 
    u32 smallest = i; 
    if (l < heap->size && heap->array[l].value < heap->array[i].value)
    {
        smallest = l;
    }
    if (r < heap->size && heap->array[r].value < heap->array[smallest].value)
    {
        smallest = r;
    }
    if (smallest != i) 
    { 
        heap_node tmp = heap->array[i];
        heap->array[i] = heap->array[smallest];
        heap->array[smallest] = tmp;
        HeapMinHeapify(heap, smallest); 
    } 
} 

global_function
heap_node
HeapExtractMin(binary_heap *heap) 
{ 
    heap_node root = heap->array[0]; 
    heap->array[0] = heap->array[--heap->size]; 
    HeapMinHeapify(heap, 0); 
  
    return(root); 
} 
  
global_variable
weighted_median_kernel **
MipMap_Weighted_Median_Kernels;

global_function
void
CreateMipMap(void *in)
{
    u32 level = *((u32 *)in);
    u16 **image = (u16 **)Images[level];
    u32 nPixels = Pixel_Resolution(Max_Image_Depth - level);
    u32 pixelResBin = 1 << level;
    weighted_median_kernel *kernel = MipMap_Weighted_Median_Kernels[level - 1];
    binary_heap *heap = kernel->heap;
    f32 **weights = kernel->weights;

    {
        u32 weightRes = pixelResBin >> 1;
        f32 total = 0.0f;
        ForLoop(weightRes)
        {
            ForLoop2(weightRes - index)
            {
                u32 dy = weightRes - index;
                u32 dx = weightRes - (index + index2);
                f32 w = 1.0f / sqrtf((f32)((dx * dx) + (dy * dy)));
                weights[index][index2] = w;
                total += index2 ? (2.0f * w) : (w);
            }
        }

        f32 norm = 1.0f / (4.0f * total);
        ForLoop(weightRes)
        {
            ForLoop2(weightRes - index)
            {
                weights[index][index2] *= norm;
            }
        }
    }

    ForLoop(nPixels)
    {
        ForLoop2(nPixels - index)
        {
            ResetBinaryHeap(heap);

            f32 totalWeight = 0.0f;

            for (   u32 pixel_x = (index + index2) * pixelResBin, binId_x = 0;
                    binId_x < pixelResBin;
                    ++pixel_x, ++binId_x )
            {
                for (   u32 pixel_y = index * pixelResBin, binId_y = 0;
                        binId_y < pixelResBin;
                        ++pixel_y, ++binId_y )
                {
                    u16 pixel = PixelLookup(pixel_x, pixel_y);
                    
                    u32 wId1 = (binId_x >= (pixelResBin >> 1)) ? (pixelResBin - 1 - binId_x) : (binId_x);
                    u32 wId2 = (binId_y >= (pixelResBin >> 1)) ? (pixelResBin - 1 - binId_y) : (binId_y);
                    u32 wId_max = Max(wId1, wId2);
                    u32 wId_min = Min(wId1, wId2);
                    f32 weight = weights[wId_min][wId_max - wId_min];

                    heap_node node;
                    node.value = (u32)pixel;
                    node.weight = weight * ((f32)pixel + (Total_Good_Reads > 1e7 ? 1.0f : (f32)1e-4));
                    totalWeight += node.weight;

                    HeapInsertKey(heap, node);
                }
            }
            
            {
                totalWeight *= 0.5f;
                f32 total = 0.0f;
                u32 value = 0;
                while (total < totalWeight)
                {
                    heap_node node = HeapExtractMin(heap);
                    value = node.value;
                    total += node.weight;
                }
                u16 val;
                if ((total - totalWeight) < (f32)1e-4)
                {
                    val = (u16)((((f32)value) + ((f32)(HeapGetMin(heap).value))) * 0.5f);
                }
                else
                {
                    val = (u16)value;
                }

                image[index][index2] = val;
            }
        }
    }
}

#define Contrast_Histogram_Cutoff_Slope 3 

global_function
void
ContrastEqualisation(void *in)
{
    u32 lodIndex = *((u32 *)in);
    u16 **image = (u16 **)Images[lodIndex];
    u32 nPixels = Pixel_Resolution(Max_Image_Depth - lodIndex);
    u32 *hist = Image_Histograms[lodIndex];
    u32 nnz = 0;

    f32 *scalars = Image_Norm_Scalars[lodIndex];

    u16 max;
    f32 maxNorm;
    u32 klIter = 0;

#define KL_Norm_Range (1 << 15)
#define KL_Norm_Tol (1e-4)
#define KL_Max_Iter 128
    do
    {
        ForLoop(nPixels)
        {
            max = 0;

            ForLoop2(nPixels)
            {
                u32 idMin = Min(index, index2);
                u32 idMax = Max(index, index2);
                u16 pixel = image[idMin][idMax - idMin];

                max = Max(max, pixel);
            }

            scalars[index] = max ? 1.0f / sqrtf((f32)max) : 1.0f; //TODO invsqrt intrinsic?
        }

        ForLoop(nPixels)
        {
            ForLoop2(nPixels - index)
            {
                image[index][index2] = (u16)((f32)image[index][index2] * scalars[index] * scalars[index + index2] * (f32)KL_Norm_Range);
            }
        }

        maxNorm = 0.0f;
        ForLoop(nPixels)
        {
            max = 0;

            ForLoop2(nPixels)
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

    ForLoop(nPixels)
    {
        ForLoop2(nPixels - index)
        {
            u16 pixel = image[index][index2];
            if (pixel)
            {
                u16 newPixel = 0;

                while (pixel)
                {
                    u32 set = HighestSetBit((u32)pixel);
                    pixel &= ~(1 << set);
                    newPixel |= (1 << (set >> 1));
                }

                image[index][index2] = newPixel;
            }
        }
    }

    max = 0;
    u16 min = u16_max;

    ForLoop(nPixels)
    {
        ForLoop2(nPixels - index)
        {
            if (index2)
            {
                u16 pixel = image[index][index2];
                if (pixel)
                {
                    max = Max(max, pixel);
                    min = Min(min, pixel);
                }
            }
        }
    }
    
    f32 scaleFactor = 254.0f / (f32)(max - min);

    ForLoop(nPixels)
    {
        ForLoop2(nPixels - index)
        {
            u16 pixel = image[index][index2];
            if (pixel)
            {
                image[index][index2] = Min((u16)((f32)(pixel - min) * scaleFactor), 254) + 1;
                if (index2)
                {
                    ++hist[image[index][index2]];
                    ++nnz;
                }
            }
        }
    }

    /*ForLoop(nPixels)
    {
        ForLoop2(nPixels - index)
        {
            if (index2)
            {
                u16 pixel = image[index][index2];
                if (pixel)
                {
                    ++hist[pixel];
                    ++nnz;
                }
            }
        }
    }*/

    if (nnz)
    {
        /*u32 nFilledBins = 0;

        ForLoop(1 << 16)
        {
            if (hist[index])
            {
                ++nFilledBins;
            }
        }*/

        u32 C = (u32)(((f32)Contrast_Histogram_Cutoff_Slope * (f32)(nnz) / 256.0f) + 0.5f);
        u32 S = 0;

        u32 top = C;
        u32 bottom = 0;

        while ((top - bottom) > 1)
        {
            u32 middle = (top + bottom) >> 1;
            S = 0;

            ForLoop(1 << 8)
            {
                if (hist[index] >= middle)
                {
                    S += (hist[index] - middle);
                }
            }

            if (S > ((C - middle) * (1 << 8)))
            {
                top = middle;
            }
            else
            {
                bottom = middle;
            }
        }

        u32 P = bottom + (u32)(((f32)(S) / (f32)(1 << 8)) + 0.5f);
        u32 L = C - P;

        ForLoop(1 << 8)
        {
            if (hist[index] < P)
            {
                hist[index] += L;
            }
            else
            {
                hist[index] = C;
            }
        }

        u32 total = 0;

        ForLoop(1 << 8)
        {
            total += hist[index];
            hist[index] = total;
        }

        ForLoop(1 << 8)
        {
            hist[index] = (u32)(((f32)(hist[index]) * (f32)((1 << 8) - 1) / (f32)(total)) + 0.5f);
        }
    }

    min = u08_max;
    max = 0;

    ForLoop(nPixels)
    {
        ForLoop2(nPixels - index)
        {
            u16 pixel = image[index][index2];
            pixel = (u16)hist[pixel];
            image[index][index2] = pixel;

            if (index2)
            {
                max = Max(max, pixel);
                min = Min(min, pixel);
            }
        }
    }

    scaleFactor = 255.0f / (f32)(max - min);

    ForLoop(nPixels)
    {
        ForLoop2(nPixels - index)
        {
            if (image[index][index2])
            {
                image[index][index2] = (u16)(((f32)image[index][index2] - (f32)min) * scaleFactor);
            }
        }
    }
}

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wconditional-uninitialized"
#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"
#pragma clang diagnostic pop

#include "TextureBufferQueue.cpp"

#define Single_Texture_Resolution 10

global_variable
texture_buffer_queue *
Texture_Buffer_Queue;

global_variable
u16 *
Texture_Coordinates;

global_variable
threadSig
Texture_Coordinate_Ptr = 0;

global_variable
u32
Number_of_Texture_Blocks = 0;

global_variable
FILE *
Output_File;

global_function
void
WriteTexturesToFile(texture_buffer *buffer)
{
    u16 myCoord = (u16)((buffer->x) << 8) | buffer->y;

    while (Texture_Coordinates[Texture_Coordinate_Ptr] != myCoord) {}
    
    fwrite(&buffer->nCommpressedBytes, 1, 4, Output_File);
    fwrite(buffer->compressionBuffer, 1, buffer->nCommpressedBytes, Output_File);
    printf("\r%3d/%3d (%1.2f%%) texture blocks written to disk...", Texture_Coordinate_Ptr + 1, Number_of_Texture_Blocks, 100.0 * (f64)((f32)(Texture_Coordinate_Ptr + 1) / (f32)Number_of_Texture_Blocks));
    fflush(stdout);

    AddTextureBufferToQueue(Texture_Buffer_Queue, buffer);
    __atomic_fetch_add(&Texture_Coordinate_Ptr, 1, 0);
}

global_function
void
CreateDXTTexture(void *in)
{
    texture_buffer *buffer = TakeTextureBufferFromQueue_Wait(Texture_Buffer_Queue);

    u16 idAll = *((u16 *)in);
    u16 id[2];
    id[0] = idAll >> 8;
    id[1] = idAll & 255;

    buffer->x = id[0];
    buffer->y = id[1];
    u32 bufferPtr = 0;

    u32 textureResolution = Single_Texture_Resolution;
    ForLoop(Number_of_LODs)
    {
        u16 **image = (u16 **)Images[index];
        u32 texturePixelResolution = Pow2(textureResolution--);

        u32 imageX = buffer->x * texturePixelResolution;
        u32 imageY = buffer->y * texturePixelResolution;

        for (   u32 x = 0;
                x < texturePixelResolution;
                x += 4 )
        {
            for (   u32 y = 0;
                    y < texturePixelResolution;
                    y += 4 )
            {
                u08 dxtInBuffer[16];
                for (   u32 dxt_x = 0, dxt_ptr = 0;
                        dxt_x < 4;
                        ++dxt_x )
                {
                    for (   u32 dxt_y = 0;
                            dxt_y < 4;
                            ++dxt_y, ++dxt_ptr)
                    {
                        u32 pixel_x = x + dxt_x + imageX;
                        u32 pixel_y = y + dxt_y + imageY;
                        u32 maxId = Max(pixel_x, pixel_y);
                        u32 minId = Min(pixel_x, pixel_y);

                        u16 pixel = image[minId][maxId - minId];
                        
                        dxtInBuffer[dxt_ptr] = (u08)pixel;
                    }
                }

                u08 dxtOut[8];
                stb_compress_bc4_block(dxtOut, (const u08 *)dxtInBuffer);
                ForLoop2(8)
                {
                    buffer->texture[bufferPtr++] = dxtOut[index2];
                }
            }
        }
    }

    if (!(buffer->nCommpressedBytes = (u32)libdeflate_deflate_compress(buffer->compressor, (const void *)buffer->texture, bufferPtr, (void *)buffer->compressionBuffer, buffer->nCommpressedBytesAvailable)))
    {
        fprintf(stderr, "Could not compress a texture info the given buffer\n");
        exit(1);
    }

    WriteTexturesToFile(buffer);
}

global_variable
const
char *
File_Name;

global_function
void
CreateDXTTextures()
{
    u32 nTextResolution = Max_Image_Depth - Single_Texture_Resolution;
    Number_of_Texture_Blocks = Pow2((nTextResolution - 1)) * (Pow2(nTextResolution) + 1);
    Texture_Coordinates = PushArray(Working_Set, u16, Number_of_Texture_Blocks);

    u32 nBytesPerText = 0;
    u32 textRes = Single_Texture_Resolution;
    ForLoop(Number_of_LODs)
    {
        nBytesPerText += Pow2((2 * textRes--));
    }
    nBytesPerText >>= 1;

    Output_File = fopen(File_Name, "wb");
    if (!Output_File)
    {
        fprintf(stderr, "Could not open output file\n");
        exit(1);
    }
    else
    {
        libdeflate_compressor *compressor = libdeflate_alloc_compressor(12);
        if (!compressor)
        {
            fprintf(stderr, "Could not allocate libdeflate compressor\n");
            exit(1);
        }
        
        u08 magic[4] = {'p', 's', 't', 'm'};
        
        u32 nBytesHeader = 15 + (68 * Number_of_Contigs); 
        u32 nBytesComp = nBytesHeader + 256;
        u08 *header = PushArray(Working_Set, u08, nBytesHeader);
        u08 *headerStart = header;
        u08 *compBuff = PushArray(Working_Set, u08, nBytesComp);

        u64 val64 = (u64)Total_Genome_Length;
        u08 *ptr = (u08 *)&val64;
        ForLoop(8)
        {
            *header++ = *ptr++;
        }
        
        u32 val32 = (u32)Number_of_Contigs;
        ptr = (u08 *)&val32;
        ForLoop(4)
        {
            *header++ = *ptr++;
        }

        ForLoop(Number_of_Contigs)
        {
            contig *cont = Contigs + index;
            f32 fracLength = (f32)((f64)cont->length / (f64)Total_Genome_Length);
            u32 *name = cont->name;
            
            ptr = (u08 *)&fracLength;
            ForLoop2(4)
            {
                *header++ = *ptr++;
            }

            ptr = (u08 *)name;
            ForLoop2(64)
            {
                *header++ = *ptr++;
            }
        }

        u08 val = Single_Texture_Resolution;
        *header++ = val;

        val = (u08)nTextResolution;
        *header++ = val;

        val = Number_of_LODs;
        *header = val;

        u32 nCommpressedBytes;
        if (!(nCommpressedBytes = (u32)libdeflate_deflate_compress(compressor, (const void *)headerStart, nBytesHeader, (void *)compBuff, nBytesComp)))
        {
            fprintf(stderr, "Could not compress file header info the given buffer\n");
            exit(1);
        }

        fwrite(magic, 1, sizeof(magic), Output_File);
        fwrite(&nCommpressedBytes, 1, 4, Output_File);
        fwrite(&nBytesHeader, 1, 4, Output_File);
        fwrite(compBuff, 1, nCommpressedBytes, Output_File);
    }

    Texture_Buffer_Queue = PushStruct(Working_Set, texture_buffer_queue);
    InitialiseTextureBufferQueue(&Working_Set, Texture_Buffer_Queue, nBytesPerText);
    u32 id = 0;
    ForLoop(Pow2(nTextResolution))
    {
        ForLoop2(Pow2(nTextResolution) - index)
        {
            Texture_Coordinates[id] = (u16)(((u16)index) << 8) | (u16)(index + index2);
            ThreadPoolAddTask(Thread_Pool, CreateDXTTexture, (void *)(Texture_Coordinates + id++));
        }
    }
}

MainArgs
{
    if (ArgCount == 1)
    {
        printf("%s\n\n", PretextMap_Version);
        printf("(...samformat, ...pairsformat |) PretextMap -o output.pretext (--sortby ({length}, name, nosort) --sortorder ({descend}, ascend) --mapq {10}) (< samfile, pairsfile)\n\n");
        printf("PretextMap --licence    <- view software licence\n");
        printf("PretextMap --thirdparty <- view third party software used\n");
        exit(0);
    }

    if (ArgCount == 2)
    {
        if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[1], (u08 *)"--licence"))
        {
            printf("%s\n", Licence);
            exit(0);
        }
        
        if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[1], (u08 *)"--thirdparty"))
        {
            printf("%s\n", ThirdParty);
            exit(0);
        }
    }
    
    u32 outputNameGiven = 0;
    for (   u32 index = 1;
            index < (u32)ArgCount;
            ++index )
    {
        if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"-o"))
        {
            ++index;
            File_Name = ArgBuffer[index];
            outputNameGiven = 1;
        }
        else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"--sortby"))
        {
            ++index;
            if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"length"))
            {
                Sort_By = sortByLength;
            }
            else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"name"))
            {
                Sort_By = sortByName;
            }
            else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"nosort"))
            {
                Sort_By = noSort;
            }
            else
            {
                fprintf(stderr, "Invalid option for sortby: %s\n", ArgBuffer[index]);
                exit(1);
            }
        }
        else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"--sortorder"))
        {
            ++index;
            if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"ascend"))
            {
                Sort_Order = ascend;
            }
            else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"descend"))
            {
                Sort_Order = descend;
            }
            else
            {
                fprintf(stderr, "Invalid option for sortorder: %s\n", ArgBuffer[index]);
                exit(1);
            }
        }
        else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"--mapq"))
        {
            ++index;
            u32 mapq;
            if (StringToInt_Check((u08 *)ArgBuffer[index], &mapq))
            {
                Min_Map_Quality = mapq;
            }
            else
            {
                fprintf(stderr, "Invalid option for mapq, not a non-negative int: %s\n", ArgBuffer[index]);
                exit(1);
            }
        }
    }

    if (!outputNameGiven)
    {
        fprintf(stderr, "Please provide an output file name (-o output.pretex)\n");
        exit(1);
    }

    InitialiseMutex(Working_Set_rwMutex);
    Number_of_Threads = 3;

    CreateMemoryArena(Working_Set, GigaByte((u64)3));
    Thread_Pool = ThreadPoolInit(&Working_Set, Number_of_Threads);

    Line_Buffer_Queue = PushStruct(Working_Set, line_buffer_queue);
    InitialiseLineBufferQueue(&Working_Set, Line_Buffer_Queue);
    InitaliseImages();

    ThreadPoolAddTask(Thread_Pool, GrabStdIn, 0);
    ThreadPoolWait(Thread_Pool);
    printf("\n");

    MipMap_Weighted_Median_Kernels = PushArray(Working_Set, weighted_median_kernel *, Number_of_LODs - 1);
    u32 mipMapLevels[Number_of_LODs];
    ForLoop(Number_of_LODs)
    {
        mipMapLevels[index] = index;
        
        if (index)
        {
            MipMap_Weighted_Median_Kernels[index - 1] = PushStruct(Working_Set, weighted_median_kernel);
            MipMap_Weighted_Median_Kernels[index - 1]->heap = InitiateBinaryHeap(&Working_Set, 1 << (2 * index));
            MipMap_Weighted_Median_Kernels[index - 1]->weights = PushArray(Working_Set, f32*, 1 << (index - 1));
            ForLoop2(1 << (index - 1))
            {
                MipMap_Weighted_Median_Kernels[index - 1]->weights[index2] = PushArray(Working_Set, f32, (1 << (index - 1)) - index2);
            }

            ThreadPoolAddTask(Thread_Pool, CreateMipMap, (void *)(mipMapLevels + index));
        }
    }
   
    printf("Creating MipMaps...\n");
    ThreadPoolWait(Thread_Pool);
    ForLoop(Number_of_LODs)
    {
        ThreadPoolAddTask(Thread_Pool, ContrastEqualisation, (void *)(mipMapLevels + index));
    }
    printf("Equalising contrast...\n");
    ThreadPoolWait(Thread_Pool);
    
    CreateDXTTextures();
    printf("Compressing textures...\n");

    ThreadPoolWait(Thread_Pool);
    ThreadPoolDestroy(Thread_Pool);
    
    printf("\n");
    fclose(Output_File);

    EndMain;
}
