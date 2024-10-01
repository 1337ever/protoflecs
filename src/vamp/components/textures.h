#pragma once
#include <flecs.h>
#include <raylib.h>

struct C_Color {
  Color color = RED;
};

struct Scale {
  float scale = 1.0f;
};

struct Rotation {
  float rot = 0.0f;
};

struct Opacity {
  float alpha = 1.0f;
  // unsigned char op = 255;
};

// render layer
struct R_Layer {
  int z = 1;
};

struct C_Texture { // raw pointer texture component, deprecated
  Texture2D *tex;
};

struct S_Texture { // lazy loaded texture component
  const char *path;
};

static inline int layer_compare(flecs::entity_t e1, const R_Layer *l1,
                                flecs::entity_t e2, const R_Layer *l2) {
  if (l1->z < l2->z) {
    return 1;
  } else if (l1->z > l2->z) {
    return -1;
  } else {
    return 0;
  }
}
