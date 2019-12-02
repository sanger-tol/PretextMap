[![Anaconda-Server Badge](https://anaconda.org/bioconda/pretext-suite/badges/installer/conda.svg)](https://conda.anaconda.org/bioconda)
[![Anaconda-Server Badge](https://anaconda.org/bioconda/pretextmap/badges/downloads.svg)](https://anaconda.org/bioconda/pretextmap)
# PretextMap
Paired REad TEXTure Mapper. Converts SAM or pairs formatted read pairs into genome contact maps. See https://github.com/4dn-dcic/pairix/blob/master/pairs_format_specification.md for pairs format specification.<br/>
Pairs format supported by version 0.04 or later only.

PretextMap is a commandline tool for converting aligned read pairs in either the SAM/BAM/CRAM or pairs format into genomic contact maps (https://github.com/aidenlab/juicer/wiki/Pre, https://higlass.io/).

Data is read from stdin over a unix pipe, eliminating the need for any intermidiate files. Alignments can be read directly from an aligner (<aligner> | PretextMap), from a SAM file (PretextMap < file.sam), from a BAM/CRAM file using samtools (samtools view -h file.bam | PretextMap) or from a pairs file (PretextMap < file.pairs). PretextMap can even be inserted into the middle of existing pipelines by using tee or similar pipe-chaining tricks.

PretextMap comes with no imposed pipeline for processing data. Process your alignments however you want before feeding to PretextMap.

# Bioconda
All commandline Pretext tools for Unix (Linux and Mac) are available on [bioconda](https://bioconda.github.io/).<br/>

The full suite of Pretext tools can be installed with
```sh
> conda install pretext-suite
```
Or, just PretextMap can be installed with
```sh
> conda install pretextmap
```

# Usage
Pipe SAM or pairs formatted read pairs to PretextMap e.g.:<br/>
samtools view -h file.bam | PretextMap<br/>
zcat file.paris.gz | PretextMap<br/>

Important: A SAM header with contig info must be present for SAM format (-h option for samtools).<br/>

Or pipe directly from an aligner e.g. bwa mem ... | PretextMap<br/>

# Options
-o specifies an output file (required)<br/>
--sortby sorts contigs by length, name or nosort (default: length)<br/>
--sortorder ascend or descend (default: descend, no effect if sortby = nosort)<br/>
--mapq sets a minimum mapping quality filter (default: 10)<br/>

New option, version 0.1:<br/>
--filterInclude: a comma separated list of sequence names, only these sequences will be included<br/>
--filterExclude: a comma separated list of sequence names, these sequence will be excluded<br/>

example: samtools view -h file.bam | PretextMap -o map.pretext --sortby length --sortorder descent --mapq 10<br/>

filtering example: samtools view -h file.bam seq_1 seq_2 | PretextMap -o map.pretext --filterInclude "seq_1, seq_2"<br/>

Filtering will increase the map resolution, since you're mapping less sequence into a fixed number of bins.<br/>
Note: also filtering with samtools view as in the above example (... seq_1 seq_2) is not nessesary, but is recommended purely for speed (provided your bam file is sorted and indexed).

# Map Format
Contact maps are saved in a compressed texture format (hence the name). Maps can be read by PretextView (https://github.com/wtsi-hpag/PretextView). Expect pretext map files to take around 30 to 50 M of disk space each.

# Requirments, running
3G of RAM and 2 CPU cores

# Windows, Mac and Linux Builds
Prebuild binaries for Windows, Mac and Linux are available<br/>
The Mac binary was build on MacOS 10.13.6<br/>
The Linux binary was build on kernel 3.13<br/>
The Windows binary was build on Windows 10, and should work on at least Windows 7<br/>
Prebuilt binaries now come in 5 different varieties: AVX2, AVX, SSE4.2, SSE4.1 and no_extensions, along with a wrapper program. Just keep all the binaries on the same path and run the wrapper (PretextMap); the correct binary for your architecture will be executed.

# Third-Party acknowledgements
PretextMap uses the following third-party libraries:<br/>
    libdeflate (https://github.com/ebiggers/libdeflate)<br/>
    mpc (https://github.com/orangeduck/mpc)<br/>
    stb_sprintf.h (https://github.com/nothings/stb/blob/master/stb_sprintf.h)<br/>
    stb_dxt.h (https://github.com/nothings/stb/blob/master/stb_dxt.h)

# Requirments, building via script (Mac and Linux only)
make<br/>
python (2 or 3) to run the installation script<br/>
clang or gcc to compile<br/>

Tested on Ubuntu linux kernel 3.13 with clang-9, gcc-4.9, gcc-5.5, gcc-8.3<br/>
Tested on MacOS 10.13.6 with clang-9, clang-10-apple<br/>

PretextMap requires libdeflate (https://github.com/ebiggers/libdeflate). By default the install script will clone and build the libdeflate.a static library for compilation with PretextMap. You can specify your own version to the install script if you wish (you'll have to specify appropriate linking flags as well if you specify a shared library).  

PretextMap requires mpc (https://github.com/orangeduck/mpc). By default the install script will clone and build the libmpc.a static library for compilation with PretextMap. You can specify your own version to the install script if you wish (you'll have to specify appropriate linking flags as well if you specify a shared library).

run ./install to build (run ./install -h to see options)

# Requirments, building on Windows
Only recomended if the prebuilt binary doesn't work for you and you know how to compile executables for Windows.<br/>

Tested on Windows 10 using the Visual Studio 2019 toolchain<br/>
Tested with Microsoft cl and clang-9<br/>

Requires libdeflate (https://github.com/ebiggers/libdeflate)<br/>
Requires mpc (https://github.com/orangeduck/mpc)<br/>

Compile PretextMap.cpp and link against libdeflate and libmpc<br/>
