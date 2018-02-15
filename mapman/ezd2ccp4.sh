#!/bin/sh

#max map size
#export MAPSIZE=356510000
export MAPSIZE=90000000
export NUMMAPS=1
export CCP4_OPEN="UNKNOWN"
file=`echo $1 | sed s/\.ezd$//`
echo "echo off"
echo "read map" > temp.txt
echo "$file.ezd" >> temp.txt
echo "NEWEZD" >> temp.txt
#echo "norm map" >> temp.txt
echo "write map" >> temp.txt
echo "$file.ccp4" >> temp.txt
echo "CCP4" >> temp.txt
echo "quit" >> temp.txt

./lx_mapman < temp.txt
echo "GZIPPING..."
pbzip2 -v $file.ccp4
pbzip2 -v $file.ezd

rm temp.txt

#11.21 MUltiply - multiply map by a constant
#11.22 DIvide - divide map by a constant
#11.23 PLus - add a constant to map
#11.26 INtegrate - sum part of map
#11.31 SImilarity - compare two maps
#11.32 COrrelate - combine two maps
