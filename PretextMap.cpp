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

#define String_(x) #x
#define String(x) String_(x)
#define PretextMap_Version "PretextMap Version " String(PV)

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

    libdeflate (https://github.com/ebiggers/libdeflate)
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

    mpc (https://github.com/orangeduck/mpc)
        Licensed Under BSD

        Copyright (c) 2013, Daniel Holden All rights reserved.

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are met:

        Redistributions of source code must retain the above copyright notice, 
        this list of conditions and the following disclaimer.
        Redistributions in binary form must reproduce the above copyright notice, 
        this list of conditions and the following disclaimer in the documentation 
        and/or other materials provided with the distribution.
        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
        AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
        IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
        ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
        FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
        (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
        LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
        ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
        (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
        THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

        The views and conclusions contained in the software and documentation are those of 
        the authors and should not be interpreted as representing official policies, either 
        expressed or implied, of the FreeBSD Project.
    
    stb_sprintf (https://github.com/nothings/stb/blob/master/stb_sprintf.h)
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


    stb_dxt (https://github.com/nothings/stb/blob/master/stb_dxt.h)
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

global_variable
u08
Status_Marco_Expression_Sponge = 0;

global_variable
char
Message_Buffer[1024];

#define PrintError(message, ...) \
{ \
stbsp_snprintf(Message_Buffer, 512, message, ##__VA_ARGS__); \
stbsp_snprintf(Message_Buffer + 512, 512, "[PretextMap error] :: %s\n", Message_Buffer); \
fprintf(stderr, "%s", Message_Buffer + 512); \
} \
Status_Marco_Expression_Sponge = 0

#define PrintStatus(message, ...) \
{ \
stbsp_snprintf(Message_Buffer, 512, message, ##__VA_ARGS__); \
stbsp_snprintf(Message_Buffer + 512, 512, "[PretextMap status] :: %s\n", Message_Buffer); \
fprintf(stdout, "%s", Message_Buffer + 512); \
} \
Status_Marco_Expression_Sponge = 0

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

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include "mpc/mpc.h"
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
    u64 length;
    u32 pad;
    u32 hashedName;
    u32 name[16];
    contig_preprocess_node *next;
};

struct
contig
{
    u64 length;
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

    while(*string != stringTerminator && stringLength < (arrayLength << 2))
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
InsertContigIntoHashTable(u32 index, u32 hash, contig_hash_table_node **table = Contig_Hash_Table)
{
    contig_hash_table_node *node = table[hash];
    contig_hash_table_node *nextNode = node ? node->next : 0;

    while (nextNode)
    {
        node = nextNode;
        nextNode = node->next;
    }
    
    contig_hash_table_node *newNode = PushStruct(Working_Set, contig_hash_table_node);
    
    newNode->index = index;
    newNode->next = 0;
    
    (node ? node->next : table[hash]) = newNode;
}

global_function
u32
ContigHashTableLookup(u32 *name, u32 nameLength, contig **cont, contig *contigs = Contigs, contig_hash_table_node **table = Contig_Hash_Table)
{
    u32 result = 1;

    contig_hash_table_node *node = table[GetHashedContigName(name, nameLength)];
    if (node)
    {
        while (!AreNullTerminatedStringsEqual(name, (contigs + node->index)->name, nameLength))
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
            *cont = contigs + node->index;
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
ProcessBodyLine(void *in)
{
    line_buffer *buffer = (line_buffer *)in;
    u08 *line = buffer->line;
    
    ForLoop(buffer->nLines)
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
            u64 relPos1 = StringToInt64(line, len);

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
                u64 relPos2 = StringToInt64(line, len);

                contig *cont = 0;
                ContigHashTableLookup(contigName1, ArrayCount(contigName1), &cont);

                if (cont)
                {
                    u64 cummLen1 = cont->previousCumulativeLength;
                    u64 read1 = cummLen1 + relPos1;

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
                        u64 read2 = cummLen2 + relPos2;

                        AddReadPairToImage(read1, read2);

                        __atomic_add_fetch(&Total_Good_Reads, 1, 0);
                    }
                }
            }
        }

        if ((nLinesTotal % StatusPrintEvery) == 0)
        {
            char buff[128];
            memset((void *)buff, ' ', 80);
            buff[80] = 0;
            printf("\r%s", buff);

            if (File_Type == sam)
                stbsp_snprintf(buff, sizeof(buff), "[PretextMap status] :: %$u reads processed, %$u read-pairs mapped", nLinesTotal, Total_Good_Reads);
            else
                stbsp_snprintf(buff, sizeof(buff), "[PretextMap status] :: %$u read-pairs mapped", nLinesTotal);

            printf("\r%s", buff);
            fflush(stdout);
        }

        while (*line++ != '\n') {}
    }

    AddLineBufferToQueue(Line_Buffer_Queue, buffer);
}

global_variable
contig *
Filter_Include_Contigs = 0;

global_variable
contig *
Filter_Exclude_Contigs = 0;

global_variable
contig_hash_table_node **
Filter_Include_Hash_Table = 0;

global_variable
contig_hash_table_node **
Filter_Exclude_Hash_Table = 0;

global_function
void
CreateFilters(const char *includeFilterString, const char *excludeFilterString)
{
    mpc_parser_t *name = mpc_new("name");
    mpc_parser_t *syntax = mpc_new("syntax");
    mpca_lang   (MPCA_LANG_DEFAULT,
            " name : /[0-9A-Za-z!#$%&\\+\\.\\/:;\\?@^_|~\\-][0-9A-Za-z!#$%&\\*\\+\\.\\/:;=\\?@^_|~\\-]*/;"
            " syntax : /^/ <name> (',' <name>)* /$/;",
            name, syntax, NULL
            );

    mpc_result_t result;

    if (includeFilterString)
    {
        if (mpc_parse("include filter", includeFilterString, syntax, &result))
        {
            mpc_ast_t *syntaxTree = (mpc_ast_t *)result.output;
            u32 nToAdd = 0;
            ForLoop((u32)syntaxTree->children_num)
            {
                if (strstr(syntaxTree->children[index]->tag, "name")) ++nToAdd;
            }

            if (nToAdd)
            {
                u32 buff[16];

                Filter_Include_Contigs = PushArray(Working_Set, contig, nToAdd);
                Filter_Include_Hash_Table = PushArray(Working_Set, contig_hash_table_node*, Contig_Hash_Table_Size);

                ForLoop(Contig_Hash_Table_Size)
                {
                    *(Filter_Include_Hash_Table + index) = 0;
                }
                u32 count = 0;
                ForLoop((u32)syntaxTree->children_num)
                {
                    if (strstr(syntaxTree->children[index]->tag, "name"))
                    {
                        PushStringIntoIntArray(buff, ArrayCount(buff), (u08 *)syntaxTree->children[index]->contents);
                        contig *cont = Filter_Include_Contigs + count;

                        ForLoop2(ArrayCount(buff))
                        {
                            cont->name[index2] = buff[index2];
                        }

                        InsertContigIntoHashTable(count++, GetHashedContigName(buff, ArrayCount(buff)), Filter_Include_Hash_Table);
                    }
                }
            }

            mpc_ast_delete((mpc_ast_t *)result.output);
        }
        else
        {
            PrintError(mpc_err_string(result.error));
            mpc_err_delete(result.error);
            exit(EXIT_FAILURE);
        }
    }

    if (excludeFilterString)
    {
        if (mpc_parse("exclude filter", excludeFilterString, syntax, &result))
        {
            mpc_ast_t *syntaxTree = (mpc_ast_t *)result.output;
            u32 nToAdd = 0;
            ForLoop((u32)syntaxTree->children_num)
            {
                if (strstr(syntaxTree->children[index]->tag, "name")) ++nToAdd;
            }

            if (nToAdd)
            {
                u32 buff[16];

                Filter_Exclude_Contigs = PushArray(Working_Set, contig, nToAdd);
                Filter_Exclude_Hash_Table = PushArray(Working_Set, contig_hash_table_node*, Contig_Hash_Table_Size);

                ForLoop(Contig_Hash_Table_Size)
                {
                    *(Filter_Exclude_Hash_Table + index) = 0;
                }
                u32 count = 0;
                ForLoop((u32)syntaxTree->children_num)
                {
                    if (strstr(syntaxTree->children[index]->tag, "name"))
                    {
                        PushStringIntoIntArray(buff, ArrayCount(buff), (u08 *)syntaxTree->children[index]->contents);
                        contig *cont = Filter_Exclude_Contigs + count;

                        ForLoop2(ArrayCount(buff))
                        {
                            cont->name[index2] = buff[index2];
                        }

                        InsertContigIntoHashTable(count++, GetHashedContigName(buff, ArrayCount(buff)), Filter_Exclude_Hash_Table);
                    }
                }
            }

            mpc_ast_delete((mpc_ast_t *)result.output);
        }
        else
        {
            PrintError(mpc_err_string(result.error));
            mpc_err_delete(result.error);
            exit(EXIT_FAILURE);
        }
    }

    mpc_cleanup(2, name, syntax);
}

global_function
u32
FilterSequenceName(u32 *name, u32 nameLength)
{
    u32 returnResult = 1;

    contig *tmp;
    if (Filter_Include_Hash_Table) 
    {
        returnResult = ContigHashTableLookup(name, nameLength, &tmp, Filter_Include_Contigs, Filter_Include_Hash_Table);
    }
    if (Filter_Exclude_Hash_Table)
    {
        returnResult = returnResult && !ContigHashTableLookup(name, nameLength, &tmp, Filter_Exclude_Contigs, Filter_Exclude_Hash_Table);
    }

    return(returnResult);
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

    if (FilterSequenceName(buff32, ArrayCount(buff32)))
    {
        line += (File_Type == sam ? 4 : 1);

        u32 count = 1;
        while (*++line != '\n' && *line != '\t')
        {
            ++count;
        }
        u64 length = StringToInt64(line, count);
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

        ++Number_of_Contigs;
        UnlockMutex(Working_Set_rwMutex);
    }
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

        if (!Number_of_Contigs)
        {
            PrintError("0 sequences to map to");
            exit(EXIT_FAILURE);
        }

        u32 *indexes = PushArray(Working_Set, u32, Number_of_Contigs);
        u32 *tmpSpace = PushArray(Working_Set, u32, Number_of_Contigs);
        contig_preprocess_node **nodes = PushArray(Working_Set, contig_preprocess_node*, Number_of_Contigs);

        u32 tmpIndex = 0;
        TraverseLinkedList((contig_preprocess_node *)First_Contig_Preprocess_Node, contig_preprocess_node)
        {
            nodes[tmpIndex] = node;
            indexes[tmpIndex] = tmpIndex;
            ++tmpIndex;

            Total_Genome_Length += node->length;
        }

        u32 hist_[512];
        u32 *hist = (u32 *)hist_;
        u32 *nextHist = hist + 256;

        for (   u32 countIndex = 0;
                countIndex < (Sort_By == noSort ? 0 : (Sort_By == sortByLength ? 2 : (ArrayCount(First_Contig_Preprocess_Node->name))));
                ++countIndex )
        {
            ForLoop(255)
            {
                hist[index] = 0;
            }

            ForLoop(Number_of_Contigs)
            {
                u32 val = Sort_By == sortByLength ? (u32)(((*(nodes+indexes[index]))->length >> (32 * countIndex)) & 0xffffffff) : (*(nodes+indexes[index]))->name[ArrayCount(First_Contig_Preprocess_Node->name) - 1 - countIndex];
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
                    u32 val = Sort_By == sortByLength ? ((u32)(((*(nodes+indexes[index]))->length >> (32 * countIndex)) & 0xffffffff) >> shift) : ((*(nodes+indexes[index]))->name[ArrayCount(First_Contig_Preprocess_Node->name) - 1 - countIndex] >> shift);
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
                u32 val = Sort_By == sortByLength ? ((u32)(((*(nodes+indexes[index]))->length >> (32 * countIndex)) & 0xffffffff) >> 24) : ((*(nodes+indexes[index]))->name[ArrayCount(First_Contig_Preprocess_Node->name) - 1 - countIndex] >> 24);
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
        u64 *lengths = PushArray(Working_Set, u64, Number_of_Contigs);
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
            u64 length = lengths[index];
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
        u32 *tmpHashes = PushArray(Working_Set, u32, 3 * Number_of_Contigs);
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

        printf("[PretextMap status] :: Mapping to %d sequences, ", Number_of_Contigs);
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

struct
read_buffer
{
    u08 *buffer;
    u64 size;
};

struct
read_pool
{
    thread_pool *pool;
    s32 handle;
    u32 bufferPtr;
    read_buffer *buffers[2];
};

global_function
read_pool *
CreateReadPool(memory_arena *arena)
{
    read_pool *pool = PushStructP(arena, read_pool);
    pool->pool = ThreadPoolInit(arena, 1);

#define ReadBufferSize MegaByte(16)
    pool->bufferPtr = 0;
    pool->buffers[0] = PushStructP(arena, read_buffer);
    pool->buffers[0]->buffer = PushArrayP(arena, u08, ReadBufferSize);
    pool->buffers[0]->size = 0;
    pool->buffers[1] = PushStructP(arena, read_buffer);
    pool->buffers[1]->buffer = PushArrayP(arena, u08, ReadBufferSize);
    pool->buffers[1]->size = 0;

    return(pool);
}

global_function
void
FillReadBuffer(void *in)
{
    read_pool *pool = (read_pool *)in;
    read_buffer *buffer = pool->buffers[pool->bufferPtr];
    buffer->size = (u64)read(pool->handle, buffer->buffer, ReadBufferSize);
}

global_function
read_buffer *
GetNextReadBuffer(read_pool *readPool)
{
    FenceIn(ThreadPoolWait(readPool->pool));
    read_buffer *buffer = readPool->buffers[readPool->bufferPtr];
    readPool->bufferPtr = (readPool->bufferPtr + 1) & 1;
    ThreadPoolAddTask(readPool->pool, FillReadBuffer, readPool);
    return(buffer);
}

global_variable
u08
Global_Error_Flag = 0;

global_function
void
GrabStdIn()
{
    line_buffer *buffer = TakeLineBufferFromQueue_Wait(Line_Buffer_Queue);
    u32 bufferPtr = 0;

    read_pool *readPool = CreateReadPool(&Working_Set);
    readPool->handle =
#ifdef DEBUG
    open("test_in", O_RDONLY);
#else
    STDIN_FILENO;
#endif
    
    u08 samLine[KiloByte(16)];
    u32 linePtr = 0;
    u32 numLines = 0;
    u08 headerMode = 1;
    read_buffer *readBuffer = GetNextReadBuffer(readPool);

    do
    {
        readBuffer = GetNextReadBuffer(readPool);
        for (   u64 bufferIndex = 0;
                bufferIndex < readBuffer->size;
                ++bufferIndex )
        {
            u08 character = readBuffer->buffer[bufferIndex];

            samLine[linePtr++] = character;
            if (linePtr == sizeof(samLine))
            {
                samLine[128] = 0;
                PrintError("SAM line too long (> %u bytes): '%s...'", sizeof(samLine), (char *)samLine);
                Global_Error_Flag = 1;
                return;
            }

            if (character == '\n')
            {
                if (headerMode)
                {
                    u08 at = samLine[0] == '@';
                    u08 hash = samLine[0] == '#';

                    if ((File_Type == sam && !at) || (File_Type == pairs && !hash)) headerMode = 0;
                    else if (File_Type == undet && !at && !hash) headerMode = 0;

                    if (!headerMode) FinishProcessingHeader();
                }
                
                if (headerMode)
                {
                    if (samLine[0] == '@' && samLine[1] == 'S')
                    {
                        if (File_Type != sam && File_Type != undet)
                        {
                            PrintError("Error, inconsistent file type");
                            Global_Error_Flag = 1;
                            return;
                        }
                        else if (File_Type == undet)
                        {
                            File_Type = sam;
                        }

                        IncramentNumberOfHeaderLines();
                        ProcessHeaderLine(samLine);
                    }
                    else if ((*((u64 *)samLine)) == 0x69736d6f72686323) // #chromsi
                    {
                        if (File_Type != pairs && File_Type != undet)
                        {
                            PrintError("Error, inconsistent file type");
                            Global_Error_Flag = 1;
                            return;
                        }
                        else if (File_Type == undet)
                        {
                            File_Type = pairs;
                        }

                        IncramentNumberOfHeaderLines();
                        ProcessHeaderLine(samLine);
                    }
                }
                else
                {
                    if ((u64)linePtr > (Line_Buffer_Size - bufferPtr))
                    {
                        buffer->nLines = numLines;
                        ThreadPoolAddTask(Thread_Pool, ProcessBodyLine, buffer);

                        buffer = TakeLineBufferFromQueue_Wait(Line_Buffer_Queue);
                        numLines = 0;
                        bufferPtr = 0;
                    }

                    ForLoop(linePtr) buffer->line[bufferPtr++] = samLine[index];
                    ++numLines;
                }

                linePtr = 0;
            }
        }
    } while (readBuffer->size);

    buffer->nLines = numLines;
    ThreadPoolAddTask(Thread_Pool, ProcessBodyLine, buffer);
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
                    node.weight = weight * ((f32)pixel + ((u64)Total_Good_Reads > (u64)1e7 ? 1.0f : (f32)1e-4));
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

    if (nnz)
    {
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
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
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
    printf("\r[PretextMap status] :: %3d/%3d (%1.2f%%) texture blocks written to disk", Texture_Coordinate_Ptr + 1, Number_of_Texture_Blocks, 100.0 * (f64)((f32)(Texture_Coordinate_Ptr + 1) / (f32)Number_of_Texture_Blocks));
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
        PrintError("Could not compress a texture info the given buffer");
        exit(EXIT_FAILURE);
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
        PrintError("Could not open output file");
        exit(EXIT_FAILURE);
    }
    else
    {
        libdeflate_compressor *compressor = libdeflate_alloc_compressor(12);
        if (!compressor)
        {
            PrintError("Could not allocate libdeflate compressor");
            exit(EXIT_FAILURE);
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
            PrintError("Could not compress file header info the given buffer");
            exit(EXIT_FAILURE);
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
        printf("\n%s\n\n", PretextMap_Version);
        
        printf(R"help(  (...samformat, ...pairsformat |)  PretextMap -o output.pretext
                                                        (--sortby ({length}, name, nosort)
                                                        --sortorder ({descend}, ascend)
                                                        --mapq {10}
                                                        --filterInclude "seq_ [, seq_]*"
                                                        --filterExclude "seq_ [, seq_]*")
  (< samfile, pairsfile))help");
        
        printf("\n\nPretextMap --licence    <- view software licence\n");
        printf("PretextMap --thirdparty <- view third party software used\n\n");
        exit(EXIT_SUCCESS);
    }

    if (ArgCount == 2)
    {
        if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[1], (u08 *)"--licence"))
        {
            printf("%s\n", Licence);
            exit(EXIT_SUCCESS);
        }
        
        if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[1], (u08 *)"--thirdparty"))
        {
            printf("%s\n", ThirdParty);
            exit(EXIT_SUCCESS);
        }
    }
    
    u32 outputNameGiven = 0;
    const char *filterIncludeString = 0;
    const char *filterExcludeString = 0;
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
                PrintError("Invalid option for sortby: \'%s\'", ArgBuffer[index]);
                exit(EXIT_FAILURE);
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
                PrintError("Invalid option for sortorder: \'%s\'", ArgBuffer[index]);
                exit(EXIT_FAILURE);
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
                PrintError("Invalid option for mapq, not a non-negative int: \'%s\'", ArgBuffer[index]);
                exit(EXIT_FAILURE);
            }
        }
        else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"--filterInclude"))
        {
            ++index;
            filterIncludeString = ArgBuffer[index];
        }
        else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"--filterExclude"))
        {
            ++index;
            filterExcludeString = ArgBuffer[index];
        }
    }

    if (!outputNameGiven)
    {
        PrintError("Please provide an output file name (-o output.pretex)");
        exit(EXIT_FAILURE);
    }

    InitialiseMutex(Working_Set_rwMutex);

    CreateMemoryArena(Working_Set, GigaByte((u64)3));
    Thread_Pool = ThreadPoolInit(&Working_Set, 3);

    Line_Buffer_Queue = PushStruct(Working_Set, line_buffer_queue);
    InitialiseLineBufferQueue(&Working_Set, Line_Buffer_Queue);
    InitaliseImages();

    CreateFilters(filterIncludeString, filterExcludeString);

    ThreadPoolAddTask(Thread_Pool, GrabStdIn, 0);
    ThreadPoolWait(Thread_Pool);
    printf("\n");
    if (Global_Error_Flag) exit(EXIT_FAILURE);

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
   
    PrintStatus("Creating MipMaps...");
    ThreadPoolWait(Thread_Pool);
    ForLoop(Number_of_LODs)
    {
        ThreadPoolAddTask(Thread_Pool, ContrastEqualisation, (void *)(mipMapLevels + index));
    }
    PrintStatus("Equalising contrast...");
    ThreadPoolWait(Thread_Pool);
    
    CreateDXTTextures();
    PrintStatus("Compressing textures...");

    ThreadPoolWait(Thread_Pool);
    ThreadPoolDestroy(Thread_Pool);
    
    printf("\n");
    fclose(Output_File);

    EndMain;
}
