#pragma once
#include <raylib.h>
#include <vector>

std::vector<Vector2> get_corners(Rectangle rect);

Vector2 *get_corners_ptr(Rectangle rect);
