#!/bin/bash

alg="discrete_light"

run () {
	for i in {1..400..60}; do
		for j in {1..400..60}; do
			curve1="../../benchmark/characters/data/${i}.txt"
			curve2="../../benchmark/characters/data/${j}.txt"

			time ../build/./calc_frechet_distance_under_translation "$curve1" "$curve2" "$alg"
		done
	done

	for i in {100..999..200}; do
		for j in {100..999..200}; do
			curve1="../../benchmark/sigspatial/file-000${i}.dat"
			curve2="../../benchmark/sigspatial/file-000${j}.dat"

			time ../build/./calc_frechet_distance_under_translation "$curve1" "$curve2" "$alg"
		done
	done

	for i in {100..999..200}; do
		for j in {100..999..200}; do
			curve1="../../benchmark/Geolife Trajectories 1.3/data/${i}.txt"
			curve2="../../benchmark/Geolife Trajectories 1.3/data/${j}.txt"

			time ../build/./calc_frechet_distance_under_translation "$curve1" "$curve2" "$alg"
		done
	done
}

time run
