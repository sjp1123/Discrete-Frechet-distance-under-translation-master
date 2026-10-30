#!/usr/bin/python3

import math
import random

n = 50
cone_deg = .3*math.pi
length_range = (1., 1.)

def get_random_point():
    return (random.uniform(-1.,1.), random.uniform(-1.,1.))

def print_point(p):
    print(str(p[0]) + " " + str(p[1]))

def step(point, cone):
    deg = random.uniform(*cone)
    length = random.uniform(*length_range)
    x = point[0] + length*math.cos(deg)
    y = point[1] + length*math.sin(deg)
    return (x, y)

def print_cone_curve(length, cone, start = (0.,0.)):
    print_point(start)
    last_point = start
    for i in range(length):
        next_point = step(last_point, cone)
        print_point(next_point)
        last_point = next_point

def get_random_cone():
    min_deg = random.uniform(0., 2*math.pi)
    max_deg = min_deg + cone_deg
    return (min_deg, max_deg)

print_cone_curve(n, get_random_cone(), get_random_point())
