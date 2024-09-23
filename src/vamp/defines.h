#pragma once

//#define ENABLE_REST

#define DRAW_COLLIDERS
#define DEBUG_UI

#define DUAL_RESTITUTION

#define W_WIDTH 1920
#define W_HEIGHT 1000

#define B_WIDTH 1920
#define B_HEIGHT 1000

#define MAX_TEXTURES 1024

#define CHARACTER_MIN_SPEED 10

//prepend this to lazy texture paths
#define TEXTURE_NAMESPACE "../resources/vamp/"
#define TEX(t) TEXTURE_NAMESPACE + t
