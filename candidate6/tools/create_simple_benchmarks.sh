#!/bin/bash

dataset="OV"

shopt -s nullglob

if [ $dataset == "sigspatial" ]; then
	filenames=(../../benchmark/sigspatial/file-0000*.dat)

	for i in {0..99}; do
		for j in {0..99}; do
			if [ $i == $j ]; then
				continue
			fi

			curve1="${filenames[$i]}"
			curve2="${filenames[$j]}"

			dist=$(../build/./calc_frechet_distance "$curve1" "$curve2" | awk '{print $5}')
			dist_plus=$(python -c "print(\"{0:.15f}\".format($dist + 0.0000001))")
			dist_minus=$(python -c "print(\"{0:.15f}\".format($dist - 0.0000001))")

			echo "$i $j $dist_plus 1"
			echo "$i $j $dist_minus 0"
		done
	done

elif [ $dataset == "OV" ]; then

	readarray -t filenames < ../../benchmark/OVinstances/dataset.txt

	for i in {0..219..2}; do
		for j in {1..219..2}; do
			curve1="../../benchmark/OVinstances/${filenames[$i]}"
			curve2="../../benchmark/OVinstances/${filenames[$j]}"

			dist=$(../build/./calc_frechet_distance "$curve1" "$curve2" | awk '{print $5}')
			dist_plus=$(python -c "print(\"{0:.15f}\".format($dist + 0.0000001))")
			dist_minus=$(python -c "print(\"{0:.15f}\".format($dist - 0.0000001))")

			echo "$i $j $dist_plus 1"
			echo "$i $j $dist_minus 0"
		done
	done
fi
