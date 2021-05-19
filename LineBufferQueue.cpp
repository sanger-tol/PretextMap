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

#define Line_Buffer_Size MegaByte(1)
#define Number_Of_Line_Buffers_Per_Queue 2
#define Number_Of_Line_Buffer_Queues 16

struct
line_buffer
{
    u08 line[Line_Buffer_Size];
    u32 homeIndex;
    u32 nLines;
    line_buffer *prev;
};

struct
single_line_buffer_queue
{
    u32 queueLength;
    u32 pad;
    mutex rwMutex;
    line_buffer *front;
    line_buffer *rear;
};

struct
line_buffer_queue
{
    single_line_buffer_queue **queues;
    threadSig index;
    u32 pad;
};

global_variable
struct line_buffer_queue *
Line_Buffer_Queue;

global_function
void
InitialiseSingleLineBufferQueue(single_line_buffer_queue *queue)
{
    InitialiseMutex(queue->rwMutex);
    queue->queueLength = 0;
}

global_function
void
AddSingleLineBufferToQueue(single_line_buffer_queue *queue, line_buffer *buffer);

global_function
void
InitialiseLineBufferQueue(memory_arena *arena, line_buffer_queue *queue)
{
    queue->queues = PushArrayP(arena, single_line_buffer_queue *, Number_Of_Line_Buffer_Queues);
    queue->index = 0;

    ForLoop(Number_Of_Line_Buffer_Queues)
    {
        queue->queues[index] = PushStructP(arena, single_line_buffer_queue);
        InitialiseSingleLineBufferQueue(queue->queues[index]);

        ForLoop2(Number_Of_Line_Buffers_Per_Queue)
        {
            line_buffer *buffer = PushStructP(arena, line_buffer);
            buffer->homeIndex = index;
            AddSingleLineBufferToQueue(queue->queues[index], buffer);
        }
    }
}

global_function
void
AddSingleLineBufferToQueue(single_line_buffer_queue *queue, line_buffer *buffer)
{
    LockMutex(queue->rwMutex);
    buffer->prev = 0;

    switch (queue->queueLength)
    {
        case 0:
            queue->front = buffer;
            queue->rear = buffer;
            break;

        default:
            queue->rear->prev = buffer;
            queue->rear = buffer;
    }

    ++queue->queueLength;
    UnlockMutex(queue->rwMutex);
}

global_function
void
AddLineBufferToQueue(line_buffer_queue *queue, line_buffer *buffer)
{
    single_line_buffer_queue *singleQueue = queue->queues[buffer->homeIndex];
    AddSingleLineBufferToQueue(singleQueue, buffer);
}

global_function
line_buffer *
TakeSingleLineBufferFromQueue(single_line_buffer_queue *queue)
{
    LockMutex(queue->rwMutex);
    line_buffer *buffer = queue->front;

    switch (queue->queueLength)
    {
        case 0:
            break;

        case 1:
            queue->front = 0;
            queue->rear = 0;
            queue->queueLength = 0;
            break;

        default:
            queue->front = buffer->prev;
            --queue->queueLength;
    }

    UnlockMutex(queue->rwMutex);

    return(buffer);
}

global_function
single_line_buffer_queue *
GetSingleLineBufferQueue(line_buffer_queue *queue)
{
    u32 index = __atomic_fetch_add(&queue->index, 1, 0) % Number_Of_Line_Buffer_Queues;
    return(queue->queues[index]);
}

global_function
line_buffer *
TakeLineBufferFromQueue(line_buffer_queue *queue)
{
    return(TakeSingleLineBufferFromQueue(GetSingleLineBufferQueue(queue)));
}

global_function
line_buffer *
TakeLineBufferFromQueue_Wait(line_buffer_queue *queue)
{
    line_buffer *buffer = 0;
    while (!buffer)
    {
        buffer = TakeLineBufferFromQueue(queue);
    }
    return(buffer);
}
