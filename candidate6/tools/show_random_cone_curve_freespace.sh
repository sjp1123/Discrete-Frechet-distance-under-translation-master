#!/bin/bash
factor="1."

./create_cone_curve.py > curve1
./create_cone_curve.py > curve2
dist=`../build/calc_frechet_distance curve1 curve2 | awk '{print $5}'`
factor_dist=`echo "print($factor*$dist)" | python3`
../build/export_freespace_diagram curve1 curve2 $factor_dist
