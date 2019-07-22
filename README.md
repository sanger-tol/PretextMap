# PretextMap
Paired REad TEXTure Mapper. Converts SAM or pairs formatted read pairs into genome contact maps. See https://github.com/4dn-dcic/pairix/blob/master/pairs_format_specification.md for pairs format specification.<br/>
Pairs format supported by version 0.04 or later only.

PretextMap is a commandline tool for converting aligned read pairs in either the SAM/BAM/CRAM or pairs format into genomic contact maps (https://github.com/aidenlab/juicer/wiki/Pre, https://higlass.io/).

Data is read from stdin over a unix pipe, eliminating the need for any intermidiate files. Alignments can be read directly from an aligner (<aligner> | PretextMap), from a SAM file (PretextMap < file.sam), from a BAM/CRAM file using samtools (samtools view -h file.bam | PretextMap) or from a pairs file (PretextMap < file.pairs). PretextMap can even be inserted into the middle of existing pipelines by using tee or similar pipe-chaining tricks.

PretextMap comes with no imposed pipeline for processing data. Process your alignments however you want before feeding to PretextMap.

# Usage
Pipe SAM or pairs formatted read pairs to PretextMap e.g. samtools view -h file.bam | PretextMap, zcat file.paris.gz | PretextMap<br/>
Important: A SAM header with contig info must be present for SAM format (-h option for samtools).<br/>
Or pipe directly from an aligner e.g. bwa mem ... | PretextMap<br/>
Note: 

# Options
-o specifies an output file (required)<br/>
--sortby sorts contigs by length, name or nosort (default: length)<br/>
--sortorder ascend or descend (default: descend, no effect if sortby = nosort)<br/>
--mapq sets a minimum mapping quality filter (default: 10)<br/>

example: samtools view -h file.bam | PretextMap -o map.pretext --sortby length --sortorder descent --mapq 10<br/>

# Map Format
Contact maps are saved in a compressed texture format (hence the name). Maps can be read by PretextView (https://github.com/wtsi-hpag/PretextView). Expect pretext map files to take around 30 to 50 M of disk space each.

# Requirments, running
3G of RAM and 2 CPU cores

# Windows, Mac and Linux Builds
Prebuild binaries for Windows, Mac and Linux are available<br/>
The Mac binary was build on MacOS 10.13.6<br/>
The Linux binary was build on kernel 3.13<br/>
The Windows binary was build on Windows 10, and should work on at least Windows 7<br/>

# Requirments, building via script (Mac and Linux only)
make<br/>
python (2 or 3) to run the installation script<br/>
clang or gcc to compile<br/>

Tested on Ubuntu linux kernel 3.13 with clang-9, gcc-4.9, gcc-5.5, gcc-8.3<br/>
Tested on MacOS 10.13.6 with clang-9, clang-10-apple<br/>

PretextMap requires libdeflate (https://github.com/ebiggers/libdeflate). By default the install script will clone and build the libdeflate.a static library for compilation with PretextMap. You can specify your own version to the install script if you wish (you'll have to specify appropriate linking flags as well if you specify a shared library).  

run ./install to build (run ./install -h to see options)

# Requirments, building on Windows
Only recomended if the prebuilt binary doesn't work for you and you know how to compile executables for Windows.<br/>

Tested on Windows 10 using the Visual Studio 2019 toolchain<br/>
Tested with Microsoft cl and clang-9<br/>

Requires libdeflate (https://github.com/ebiggers/libdeflate)<br/>

Compile PretextMap.cpp and link against libdeflate<br/>
