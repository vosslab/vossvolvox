#!/bin/sh

export MAPSIZE=90000000
export CCP4_OPEN="UNKNOWN"
echo "echo off"
echo "read map" > temp.txt
echo "$1.ccp4" >> temp.txt
echo "CCP4" >> temp.txt
#echo "norm map" >> temp.txt
echo "write map" >> temp.txt
echo "$1.ezd" >> temp.txt
echo "NEWEZD" >> temp.txt
echo "quit" >> temp.txt

./lx_mapman < temp.txt

rm temp.txt
