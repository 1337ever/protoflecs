#pragma once
#include <raylib.h>
#include <vector>

struct Speed {
  float speed;
};

struct Position {
  Vector2 pos = Vector2{0, 0};
};

struct Velocity {
  Vector2 vel = Vector2{0, 0};
  float max;
};

// velocity damping, for simulating ie friction
struct Damping {
  float damp; // current damping
  float def;  // default damping
};

enum Collision { left, right, top, bottom };

// rect colliders only!!
struct Collider {
  Rectangle box;
  bool solid = true;
  int layer = 1;
  bool colliding = false;
  std::vector<Collision> collisions;
};
