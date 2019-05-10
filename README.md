# PretextMap
Paired REad TEXTure Mapper. Converts SAM formatted read pairs into genome contact maps.

PretextMap is a commandline tool for converting aligned read pairs in the SAM/BAM/CRAM format into genomic contact maps (https://github.com/aidenlab/juicer/wiki/Pre, https://higlass.io/).

Data is read from stdin over a unix pipe, eliminating the need for any intermidiate files. Alignments can be read directly from an aligner (<aligner> | PretextMap), from a SAM file (PretextMap < file.sam) or from a BAM/CRAM file using samtools (samtools view -h file.bam | PretextMap). PretextMap can even be inserted into the middle of existing pipelines by using tee or similar pipe-chaining tricks.

PretextMap comes with no imposed pipeline for processing data. Process your alignments however you want before feeding to PretextMap.

# Usage
Pipe SAM formatted read pairs to PretextMap e.g. samtools view -h file.bam | PretextMap. Important: A SAM header with contig info must be present (-h option for samtools). Or pipe directly from an aligner e.g. bwa mem ... | PretextMap

# Options
-o specifies an output file (required)
--sortby sorts contigs by length, name or nosort (default: length)
--sortorder ascend or descend (default: descend, no effect if sortby = nosort)
--mapq sets a minimum mapping quality filter (default: 10)

example: samtools view -h file.bam | PretextMap -o map.pretext --sortby length --sortorder descent --mapq 10

# Map Format
Contact maps are saved in a compressed texture format (hence the name). Maps can be read by PretextView. Expect pretext map files to take around 30 to 50 M of disk space each.

# Requirments, running
3G of RAM and 2 CPU cores

# Requirments, installation
make
python (2 or 3) to run the installation script
clang or gcc to compile

Tested on Ubuntu linux kernel 3.13 with clang-9, gcc-4.9, gcc-5.5, gcc-8.3
Tested on MacOS 10.13.6 with clang-9, clang-10-apple

PretextMap requires libdeflate (https://github.com/ebiggers/libdeflate). By default the install script will clone and build the libdeflate.a static library for compilation with PretextMap. You can specify your own version to the install script if you wish (you'll have to specify appropriate liking flags as well if you specify a shared library).

run ./install to build (run ./install -h to see options)
