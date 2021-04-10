#!/usr/bin/env bash

set -eo pipefail

samtools view -h ../Test/test.cram | ./PretextMap -o test.map
diff test.map ../Test/test.map

