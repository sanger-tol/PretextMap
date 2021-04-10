[![Travis_CI](https://travis-ci.org/wtsi-hpag/PretextMap.svg?branch=master)](https://travis-ci.org/github/wtsi-hpag/PretextMap)
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

# Third-Party acknowledgements
PretextMap uses the following third-party libraries:<br/>
* [libdeflate](https://github.com/ebiggers/libdeflate)<br/>
* [mpc](https://github.com/orangeduck/mpc)<br/>
* [stb_sprintf.h](https://github.com/nothings/stb/blob/master/stb_sprintf.h)<br/>
* [stb_dxt.h](https://github.com/nothings/stb/blob/master/stb_dxt.h)

# Installation
Requires:
git submodule update --init --recursive
* clang >= 11.0.0
* meson >= 0.57.1
```bash
env CXX=clang meson setup --buildtype=release --unity on --prefix=<installation prefix> builddir
cd builddir
meson compile
meson test
meson install
```
