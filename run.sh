#! /bin/bash

#$ -q large.q
#$ -cwd
#$ -V
#$ -l "h_rt=24:00:00"
#$ -o output3.my
#$ -e error3.my

./a.out
