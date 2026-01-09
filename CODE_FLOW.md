# PretextMap Code Flow and Logic

## Overview
PretextMap converts paired-end sequencing data (SAM or pairs format) into a contact map visualization file (.pretext format). It processes read pairs, builds a contact matrix, generates multi-resolution images, and compresses them into textures.

---

## Main Program Flow

### 1. **Initialization Phase** (MainArgs function, lines 2312-2561)

```
┌─────────────────────────────────────┐
│  Parse Command-Line Arguments        │
│  -o output.pretext                   │
│  --sortby (length|name|nosort)       │
│  --sortorder (ascend|descend)        │
│  --mapq <quality>                    │
│  --filterInclude "seq1,seq2,..."     │
│  --filterExclude "seq1,seq2,..."     │
│  --highRes                           │
│  [input_file]                        │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  Initialize Resources                │
│  - Create memory arena (3GB/16GB)    │
│  - Initialize thread pool (3 threads) │
│  - Initialize mutexes                 │
│  - Create line buffer queue           │
│  - Initialize image arrays            │
│  - Create contig filters              │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  Open Input File                     │
│  - If specified: open and redirect   │
│    to stdin                          │
│  - Otherwise: read from stdin         │
└─────────────────────────────────────┘
```

---

### 2. **Input Reading Phase** (GrabStdIn function, lines 1349-1622)

```
┌─────────────────────────────────────┐
│  Create Read Pool                   │
│  - Allocate 2 buffers (16MB each)   │
│  - Set up double buffering           │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  Read Input Character by Character   │
│  - Handle UTF-8 BOM if present       │
│  - Build lines until newline         │
│  - Detect file format:               │
│    • SAM: starts with '@'            │
│    • Pairs: starts with '#'          │
└─────────────────────────────────────┘
              │
              ▼
        ┌─────┴─────┐
        │           │
        ▼           ▼
┌───────────┐  ┌───────────┐
│ Header    │  │ Body     │
│ Mode      │  │ Mode     │
│ (lines    │  │ (data    │
│ starting  │  │ lines)   │
│ with #/@) │  │          │
└───────────┘  └───────────┘
```

---

### 3. **Header Processing Phase** (ProcessHeaderLine function, lines 794-971)

#### For Pairs Format:
```
┌─────────────────────────────────────┐
│  Process Header Lines               │
│                                      │
│  Line Types:                         │
│  1. "## pairs format v1.0"          │
│     → Skip (format declaration)     │
│                                      │
│  2. "#columns: ..."                  │
│     → Skip (column names)            │
│                                      │
│  3. "#chromsize: <name> <length>"    │
│     → Parse contig info              │
│     → Apply filters (include/exclude)│
│     → Add to contig list             │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  FinishProcessingHeader()            │
│  - Wait for all header lines         │
│  - Build contig array                │
│  - Calculate cumulative lengths       │
│  - Sort contigs (if requested)       │
│  - Create hash table for lookup       │
└─────────────────────────────────────┘
```

**Key Data Structures:**
- `contig_preprocess_node`: Linked list of contigs during parsing
- `contig`: Final sorted array with cumulative positions
- `Contig_Hash_Table`: Hash table for fast contig name lookup

---

### 4. **Data Line Processing Phase** (ProcessBodyLine function, lines 497-640)

```
┌─────────────────────────────────────┐
│  Parse Each Data Line               │
│  (Tab-separated fields)              │
│                                      │
│  Expected Fields:                    │
│  1. readID (skipped)                 │
│  2. chr1 (contig name for read 1)    │
│  3. pos1 (position on chr1)          │
│  4. chr2 (contig name for read 2)    │
│  5. pos2 (position on chr2)           │
│  6. strand1 (skipped)                 │
│  7. strand2 (skipped)                 │
│  ... (any additional fields skipped)  │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  Validate and Process                │
│  - Look up contig1 in hash table     │
│  - Look up contig2 in hash table     │
│  - Validate positions < contig length│
│  - Calculate absolute positions:      │
│    pos1_abs = contig1.cumulative + pos1│
│    pos2_abs = contig2.cumulative + pos2│
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  Add to Contact Matrix               │
│  AddReadPairToImage(pos1_abs, pos2_abs)│
│  - Convert to pixel coordinates        │
│  - Increment pixel value atomically   │
│  - Track total reads processed        │
└─────────────────────────────────────┘
```

**Image Structure:**
- Multi-resolution pyramid: 10-15 levels (or 10-16 in high-res mode)
- Each level is a triangular matrix (only stores upper triangle)
- Base level: 2^15 = 32,768 pixels (or 2^16 = 65,536 in high-res)

---

### 5. **Image Processing Phase**

#### 5.1 Mipmap Generation (CreateMipMap function, lines 1748-1841)

```
┌─────────────────────────────────────┐
│  For Each LOD Level (1 to N-1)      │
│                                      │
│  Create Lower Resolution from Higher │
│  - Use weighted median kernel         │
│  - Distance-weighted averaging        │
│  - Binary heap for median calculation │
└─────────────────────────────────────┘
```

**Mipmap Levels:**
- Level 0: Highest resolution (base image)
- Level 1: 2x downsampled
- Level 2: 4x downsampled
- ...
- Level N-1: Lowest resolution

#### 5.2 Contrast Equalization (ContrastEqualisation function, lines 1847-2060)

```
┌─────────────────────────────────────┐
│  For Each LOD Level                  │
│                                      │
│  1. Normalize by row/column max      │
│     - Calculate max per row           │
│     - Apply 1/sqrt(max) scaling      │
│     - Iterate until convergence       │
│                                      │
│  2. Bit reduction                     │
│     - Reduce bit depth                │
│                                      │
│  3. Histogram equalization            │
│     - Build histogram                 │
│     - Apply contrast enhancement      │
│     - Map to 0-255 range               │
└─────────────────────────────────────┘
```

---

### 6. **Texture Compression Phase** (CreateDXTTextures function, lines 2197-2310)

```
┌─────────────────────────────────────┐
│  Write File Header                   │
│  - Magic number: "pstm"               │
│  - Total genome length                │
│  - Number of contigs                  │
│  - Contig info (name, fraction)       │
│  - Texture resolution parameters      │
│  - Compress header with deflate       │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  For Each Texture Block              │
│  (CreateDXTTexture function)         │
│                                      │
│  1. Extract texture from all LODs    │
│  2. Convert to BC4 (DXT5 alpha) format │
│     - 4x4 pixel blocks                │
│     - Compress each block             │
│  3. Compress entire texture with      │
│     deflate                           │
│  4. Write to output file              │
└─────────────────────────────────────┘
```

---

## Key Data Structures

### Contig Management
```cpp
struct contig {
    u64 length;                    // Contig length
    u64 previousCumulativeLength;   // Sum of all previous contigs
    u32 name[16];                  // Contig name (64 bytes)
};
```

### Image Storage
```cpp
Images[LOD_index][x][y-x]  // Triangular matrix storage
// Only stores upper triangle: y >= x
// Value at (x,y) = Images[0][min(x,y)][max(x,y)-min(x,y)]
```

### Threading Model
- **Thread Pool**: 3 worker threads
- **Line Buffer Queue**: Producer-consumer pattern for data lines
- **Texture Buffer Queue**: Producer-consumer pattern for textures
- **Atomic Operations**: Used for pixel increments and counters

---

## File Format Support

### Pairs Format
```
## pairs format v1.0
#chromsize: contig1 length1
#chromsize: contig2 length2
...
#columns: readID chr1 pos1 chr2 pos2 strand1 strand2
read1    chr1    pos1    chr2    pos2    +    -
...
```

### SAM Format
```
@SQ    SN:contig1    LN:length1
@SQ    SN:contig2    LN:length2
...
read1    0    chr1    pos1    ...
```

---

## Memory Management

- **Memory Arena**: Custom allocator for efficient memory management
- **Working Set**: Main memory arena (3GB default, 16GB high-res)
- **Double Buffering**: Used for input reading and texture processing
- **Line Buffers**: Reusable buffers for data line processing

---

## Performance Optimizations

1. **Multi-threading**: Parallel processing of data lines and textures
2. **Triangular Matrix**: Only stores upper triangle (saves 50% memory)
3. **Mipmaps**: Pre-computed lower resolutions for fast zooming
4. **Texture Compression**: BC4 + deflate for efficient storage
5. **Hash Tables**: Fast contig name lookup
6. **Atomic Operations**: Lock-free pixel increments

---

## Error Handling

- **Global_Error_Flag**: Set on errors, checked at key points
- **Validation**: Contig names, positions, file format
- **Status Messages**: PrintStatus/PrintError macros for logging
- **Graceful Degradation**: Continues processing valid reads, skips invalid ones

---

## Output Format (.pretext)

1. **Header** (compressed):
   - Magic: "pstm"
   - Genome length
   - Contig information
   - Texture parameters

2. **Texture Blocks** (compressed):
   - Each block contains all LOD levels
   - BC4 compressed 4x4 pixel blocks
   - Deflate compressed entire texture




