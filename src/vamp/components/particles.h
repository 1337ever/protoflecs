#pragma once
#include <raylib.h>

struct Emitter {
  const char *icon = "../resources/vamp/particles/heart.png";
  float rate = 0.1;
  float lifetime = 0.5;
  float r_spread = 30.;
  float r_rot = 10.;
  float r_scale = 0.5;
  Vector2 r_velocity = {0, 0};
  double time = 0.; // not rlly meant to be set, just keeps track of time
};

struct Emitted {};

// component for holding and checking time
struct Time {
  double time = 0;
};

struct Lifetime {
  double time = 0;
};

struct Particle {};
