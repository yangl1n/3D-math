#! /bin/bash

#$ -q large.q
#$ -cwd
#$ -V
#$ -l "h_rt=00:10:00"
#$ -o output.my
#$ -e error.my

./a.out
