#include <vector>
#include <raylib.h>
#include "physics.h"

std::vector<Vector2> get_corners(Rectangle rect) {
  std::vector<Vector2> ret = {};
  ret.push_back(Vector2{rect.x, rect.y});
  ret.push_back(Vector2{rect.x + rect.width, rect.y});
  ret.push_back(Vector2{rect.x, rect.y + rect.height});
  ret.push_back(Vector2{rect.x + rect.width + rect.width, rect.y});

  return ret;
}

Vector2 *get_corners_ptr(Rectangle rect) {
  static Vector2 ret[4];
  ret[0] = Vector2{rect.x, rect.y};
  ret[1] = Vector2{rect.x + rect.width, rect.y};
  ret[2] = Vector2{rect.x, rect.y + rect.height};
  ret[3] = Vector2{rect.x + rect.width + rect.width, rect.y};
  return ret;
}
