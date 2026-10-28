#!/usr/bin/python3

import math

clockwise = False

if clockwise:
    for deg in range(361):
        rad = deg/360.*2.*math.pi
        print(str(math.cos(-rad)) + " " + str(math.sin(-rad)))
else:
    for deg in range(361):
        rad = deg/360.*2.*math.pi
        print(str(math.cos(rad)) + " " + str(math.sin(rad)))
