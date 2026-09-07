#!/bin/bash
D="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
bash "$D/_build_one.sh" candidate
bash "$D/_build_one.sh" candidate2
echo "=== ALL REST DONE ==="
ls -la "$HOME"/b_candidate/calc_frechet_distance_under_translation "$HOME"/b_candidate2/calc_frechet_distance_under_translation 2>&1
