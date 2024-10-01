#pragma once

struct Player {};

struct Enemy {}; // runs toward and attacks player

struct Octophant {};

struct Afraid {}; // runs away from player

struct Ephemeral {
  float buffer = 32;
}; // despawns if out of bounds by buffer degree

struct Health {
  int health = 100;
};
