#pragma once

#include "curves.h"

#include <cmath>

using Angle = float;

int calcConeChanges(Curve const& curve, Angle cone_angle = M_PI - 10e-10);
