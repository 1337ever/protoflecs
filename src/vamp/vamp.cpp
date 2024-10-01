#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <vector>

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#include "subprojects/reasings/src/reasings.h"

#include "subprojects/rlImGui/imgui-master/imgui.h"
#include "subprojects/rlImGui/rlImGui.h"

#include "aabb.cpp"
#include "defines.h"
#include "helpers/physics.h"
#include "helpers/math.h"

#include "components/particles.h"
#include "components/octophant.h"
#include "components/physics.h"
#include "components/textures.h"
// #include "textures.cpp"

int main() {
  flecs::world ecs;

  // vvvv for some reason this causes a really hard to diagnose runtime error
  // ecs.set_threads(4);

#ifdef ENABLE_REST
  ecs.set<flecs::Rest>({});
// ecs.import<flecs::monitor>();
#endif

  InitWindow(W_WIDTH, W_HEIGHT, "Octophant Scimitar");

#ifdef DEBUG_UI
  rlImGuiSetup(true);
#endif

  // create lazy texture library
  std::unordered_map<const char *, Texture2D> TextureLibrary;

  auto player = ecs.entity()
                    .set<Position>({W_WIDTH / 2, W_HEIGHT / 2})
                    .set<Scale>({3})
                    .set<Rotation>({0})
                    .set<Velocity>({{0, 0}, 350})
                    .set<Damping>({0, 5})
                    .set<Speed>({5000})
                    .set<S_Texture>({"../resources/vamp/char1.png"})
                    .set<R_Layer>({1})
                    //.set<Collider>({{-8, 8, -8, 8}})
                    .set<Collider>({{0, 0, 16 * 3, 16 * 3}})
                    .add<Player>()
                    //.add<Emitter>()
                    .add<Octophant>();

  auto healophant = ecs.entity()
                        .set<Position>({W_WIDTH / 2, W_HEIGHT / 2})
                        .set<Scale>({3})
                        .set<Rotation>({0})
                        .set<Velocity>({{0, 0}, 350})
                        .set<Damping>({0, 5})
                        .set<Speed>({200})
                        .set<S_Texture>({"../resources/vamp/healophant.png"})
                        .set<R_Layer>({1})
                        //.set<Collider>({{-8, 8, -8, 8}})
                        .set<Collider>({{0, 0, 16 * 3, 16 * 3}})
                        .add<Afraid>()
                        .add<Ephemeral>()
                        .set<Emitter>({"../resources/vamp/particles/heal.png",
                                       0.05,
                                       0.5,
                                       50.,
                                       10.,
                                       0.5,
                                       {50., 50.}})
                        .add<Octophant>();

  auto heart = ecs.entity()
                   .set<Position>({W_WIDTH * 0.5, 32 * 3})
                   .set<Scale>({5})
                   .set<Rotation>({0})
                   .set<S_Texture>({"../resources/vamp/heart.png"})
                   .set<Opacity>({0.8})
                   .set<R_Layer>({0});

  auto scimi = ecs.entity()
                   .set<Position>({W_WIDTH / 2, W_HEIGHT / 2})
                   .set<Scale>({3})
                   .set<Rotation>({0})
                   .set<S_Texture>({"../resources/vamp/scimi2.png"})
                   .set<Collider>({{0, 0, 16 * 3, 16 * 3}, false})
                   .set<R_Layer>({0});

#ifdef DRAW_COLLIDERS
  auto sys_draw_colliders =
      ecs.system<const Collider, const Position, const Scale>()
          //.multi_threaded()
          //^^ uncomment for 666 spooky
          .each([](const Collider &c, const Position &p, const Scale &s) {
            float half_width = c.box.width / 2;
            float half_height = c.box.height / 2;
            // if (c.collisions.size() > 0)
            DrawRectangleLinesEx({c.box.x, c.box.y, c.box.width, c.box.height},
                                 2.0, c.colliding ? RED : BLUE);
          });
#endif

  auto sys_run_emitter =
      ecs.system<const Position, Emitter, const Velocity *>().each(
          [&ecs](const Position &p, Emitter &e, const Velocity *v) {
            auto time = GetTime();
            if (time - e.time >= e.rate) {
              // if velocity component exists, use it
              Vector2 vel = v ? Vector2{v->vel.x, v->vel.y} : Vector2{0, 0};
              float x =
                  float_rand(-e.r_velocity.x - vel.x, e.r_velocity.x + vel.x);
              float y =
                  float_rand(-e.r_velocity.y - vel.y, e.r_velocity.y + vel.y);

              auto particle =
                  ecs.entity()
                      .set<S_Texture>({e.icon})
                      .set<Position>(
                          {p.pos.x + GetRandomValue(-e.r_spread, e.r_spread),
                           p.pos.y + GetRandomValue(-e.r_spread, e.r_spread)})
                      .set<Rotation>({static_cast<float>(
                          GetRandomValue(-e.r_rot, e.r_rot))})
                      .set<R_Layer>({0})
                      .set<Opacity>({1.0f})
                      //.add<Afraid>()
                      .set<Speed>({700})
                      .set<Velocity>({{x, y}, 350})
                      .set<Damping>({0, 0})
                      .add<Particle>()
                      // vv this breaks when multithreading
                      // i think because the particles are being deleted
                      // and i'm not checking .is_alive before trying to access data
                      //.set<Lifetime>({e.lifetime})
                      //.add<Time>()
                      // vvv this doesnt really work
                      //.set<Collider>({{0, 0, 5 * 3, 5 * 3}, true, 1})
                      .set<Scale>({static_cast<float>(
                          3 + GetRandomValue(-e.r_scale, e.r_scale))});
              e.time = time;
            }
          });

  auto sys_process_particles = ecs.system<Opacity>().with<const Particle>().each(
      [&ecs](flecs::entity e1, Opacity &o) {
        if (o.alpha <= 0) {
          e1.destruct();
        } else {
          o.alpha -= 0.8f * ecs.delta_time();
        };
      });

  // sync collider positions, annoying this is separate
  auto sys_sync_colliders =
      ecs.system<const Position, Collider, const Scale>().multi_threaded().each(
          [](const Position &p, Collider &c, const Scale &s) {
            float half_width = (c.box.width) / 2;
            float half_height = (c.box.height) / 2;
            c.box.x = p.pos.x - half_width;
            c.box.y = p.pos.y - half_height;
          });

  auto sys_vel = ecs.system<Position, const Velocity>().multi_threaded().each(
      [&ecs](Position &p, const Velocity &v) {
        float delta = ecs.delta_time();
        p.pos.x += v.vel.x * delta;
        p.pos.y += v.vel.y * delta;
      });

  auto sys_collision_resolve =
      ecs.system<Position, Collider>()
          .multi_threaded()
          // this double .each syntax scares me so much i have no idea why....
          // vvvv
          .each([&ecs](flecs::entity e1, Position &pos1, Collider &col1) {
            // this `.each` doesn't accept `<Position, Collider>`. Oh well
            ecs.each<Collider>([&](flecs::entity e2, Collider &col2) {
              // v prevents double-processing a pair of objects
              if (e1.id() > e2.id()) {
                if (col1.layer == col2.layer) {
                  if (CheckCollisionRecs(col1.box, col2.box)) {
                    if (col1.solid && col2.solid) {
                      // hacky inefficient
                      ecs.each<Position>([&](flecs::entity e3, Position &pos2) {
                        if (e3.id() == e2.id()) {
                          std::vector<Vector2> cornerListA =
                              get_corners(col1.box);
                          std::vector<Vector2> cornerListB =
                              get_corners(col2.box);

                          int sharedcornerindex = 0;
                          for (int i = 0; i < 4; i++) {
                            if (cornerListA[i].x == cornerListB[i].x &&
                                cornerListA[i].y == cornerListB[i].y) {
                              sharedcornerindex++;
                              break;
                            }
                          }

                          int oppositecornerindex = 0;
                          if (sharedcornerindex == 0)
                            oppositecornerindex = 3;
                          if (sharedcornerindex == 3)
                            oppositecornerindex = 0;
                          if (sharedcornerindex == 1)
                            oppositecornerindex = 2;
                          if (sharedcornerindex == 2)
                            oppositecornerindex = 1;

                          Vector2 v = Vector2{};
                          v.x = cornerListB[oppositecornerindex].x -
                                cornerListA[oppositecornerindex].x;
                          v.y = cornerListB[oppositecornerindex].y -
                                cornerListA[oppositecornerindex].y;

                          float mag = 40;
                          float delta = ecs.delta_time();

                          // todo: replace 3 with scale
                          pos1.pos.x -= ((v.x / 3) * mag) * delta;
                          pos1.pos.y -= ((v.y / 3) * mag) * delta;

#ifdef DUAL_RESTITUTION
                          pos2.pos.x += ((v.x / 3) * mag) * delta;
                          pos2.pos.y += ((v.y / 3) * mag) * delta;
#endif
                          // pos2.pos.x += v.x/3/2;
                          // pos2.pos.y += v.y/3/2;
                        }
                      });
                    }

                    // boolean colliding doesn't really work if there's
                    //>2 colliders
                    col1.colliding = true;
                    col2.colliding = true;
                  } else {
                    col1.colliding = false;
                    col2.colliding = false;
                  }
                }
              }
            });
          });

  // apply a cute velocity-driven rotation to the sprite
  auto anim_octophant =
      ecs.system<const Velocity, Rotation>()
          .multi_threaded()
          .with<Octophant>()
          .each([](const Velocity &v, Rotation &r) {
            float maxrot = 10.0f; // in degrees
            r.rot = Remap(v.vel.x, -v.max, v.max, -maxrot, maxrot);
          });

  auto sys_damp = ecs.system<Velocity, const Damping>().multi_threaded().each(
      [&ecs](Velocity &v, const Damping &d) {
        float damp = d.damp * ecs.delta_time();

        v.vel.x -= damp * v.vel.x;
        v.vel.y -= damp * v.vel.y;

        v.vel.x = std::clamp(v.vel.x, -v.max, v.max);
        v.vel.y = std::clamp(v.vel.y, -v.max, v.max);
      });

  auto sys_lazy_load = ecs.system<const S_Texture>().multi_threaded().each(
      [&TextureLibrary](flecs::entity e1, const S_Texture &st) {
        // if texture not loaded, load it into the library
        if (TextureLibrary.find(st.path) == TextureLibrary.end()) {
          printf("lazy loading texture @%s\n", st.path);
          TextureLibrary[st.path] = LoadTexture(st.path);
        }
        // insert a new C_Texture with a reference to the corresponding texture
        // in library
        e1.remove<C_Texture>(); // remove any existing C_Texture, to allow
                                // textures to be easily changed via the
                                // insertion of S_Textures
        e1.set<C_Texture>({&TextureLibrary[st.path]});
        e1.remove<S_Texture>();
      });

  auto sys_draw_sorted =
      ecs.system<const Position, const Scale, const Rotation, const C_Texture,
                 const R_Layer, const Opacity *>()
          .kind(flecs::OnUpdate)
          .order_by<R_Layer>(layer_compare) // absolute life saver
          .each([](const Position &p, const Scale &s, const Rotation &r,
                   const C_Texture &ct, const R_Layer &l, const Opacity *o) {
            Texture2D *tex = ct.tex;

            int nw = tex->width;
            int nh = tex->height;

            // detect presence of optional component Opacity
            float opacity = o ? o->alpha : 1.0f;
            Color faded = Fade(WHITE, opacity);
            DrawTexturePro(*tex, {0.0f, 0.0f, (float)nw, (float)nh},
                           {p.pos.x, p.pos.y, nw * s.scale, nh * s.scale},
                           {(nw * s.scale) / 2, (nh * s.scale) / 2}, r.rot,
                           faded);
          });

  auto sys_move_enemy =
      ecs.system<const Position, Velocity, const Speed, const Enemy>()
          .multi_threaded()
          .each([&player](const Position &p, Velocity &v, const Speed &s,
                          const Enemy &e) {
            const Vector2 p_pos = player.get<Position>()->pos;
            // these functions look uggo
            Vector2 dir =
                Vector2Multiply(Vector2Normalize(Vector2Subtract(p_pos, p.pos)),
                                {s.speed, s.speed});
            v.vel = dir;
          });

  auto sys_move_afraid =
      ecs.system<const Position, Velocity, const Speed, const Afraid>()
          .multi_threaded()
          .each([&player](const Position &p, Velocity &v, const Speed &s,
                          const Afraid &a) {
            const Vector2 p_pos = player.get<Position>()->pos;
            // these functions look uggo
            Vector2 dir =
                Vector2Multiply(Vector2Normalize(Vector2Subtract(p_pos, p.pos)),
                                {s.speed, s.speed});
            v.vel = Vector2Multiply(dir, Vector2{-1, -1});
          });

  auto sys_despawn_ephemeral =
      ecs.system<const Position, const Ephemeral>().multi_threaded().each(
          [](flecs::entity e1, const Position &p, const Ephemeral &e) {
            if (p.pos.x < 0 - e.buffer) {
              e1.destruct();
            }
            if (p.pos.x > B_WIDTH + e.buffer) {
              e1.destruct();
            }
            if (p.pos.y > B_HEIGHT + e.buffer) {
              e1.destruct();
            }
            if (p.pos.y < 0 - e.buffer) {
              e1.destruct();
            }
            // printf("destructing entity %s\n", e1);
          });

  auto sys_character_controller =
      ecs.system<Velocity, Damping, const Speed, const Player>()
          .multi_threaded()
          .each([](Velocity &v, Damping &d, const Speed &s, const Player &p) {
            float delta = GetFrameTime();
            float speed = s.speed * delta;
            bool moving = false;
            if (IsKeyDown(KEY_A)) {
              v.vel.x -= speed;
              moving = true;
            }
            if (IsKeyDown(KEY_D)) {
              v.vel.x += speed;
              moving = true;
            }
            if (IsKeyDown(KEY_S)) {
              v.vel.y += speed;
              moving = true;
            }
            if (IsKeyDown(KEY_W)) {
              v.vel.y -= speed;
              moving = true;
            }

            if (moving) {
              d.damp = 0;
            } else {
              d.damp = d.def;

              // refine so that each axis is minspeeded separately (if one axis
              // is no longer moving) also each axis needs to be damped
              // separately for similar reasons
              float minspeed = CHARACTER_MIN_SPEED;
              if (v.vel.x < minspeed && !(v.vel.x < -minspeed)) {
                v.vel.x = 0;
              }
              if (v.vel.y < minspeed && !(v.vel.y < -minspeed)) {
                v.vel.y = 0;
              }
            }
          });

  int count = 0;
  int total = 0;
  int evil_per_check = 1;
  int interval = 5;
  while (!WindowShouldClose() && ecs.progress(GetFrameTime())) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // spawn an evilophant in a random location every 5 seconds
    // todo: make them spawn offscreen
    bool spawn = (int)GetTime() % interval ? false : true;
    if (spawn && count < evil_per_check) {
      ecs.entity()
          .set<Position>(
              {(float)GetRandomValue(0, W_WIDTH), (float)GetRandomValue(0, W_HEIGHT)})
          .set<Scale>({3})
          .set<Rotation>({0.0})
          .set<Velocity>({{0, 0}, 350})
          .set<Speed>({200})
          .set<S_Texture>({"../resources/vamp/sickophant.png"})
          .set<R_Layer>({2})
          .set<Collider>({{0, 0, 16 * 3, 16 * 3}})
          .add<Enemy>()
          .set<Emitter>({"../resources/vamp/particles/sick.png", 0.4, 0.5, 40.})
          .set<Ephemeral>({300.})
          .add<Octophant>();
      count++;
      total++;
    } else if (!spawn) {
      count = 0;
    }

#ifdef DEBUG_UI
    rlImGuiBegin();

    ImGui::Begin("spawn dialog");
    ImGui::SliderInt("spawn interval", &interval, 1, 25);
    ImGui::SliderInt("# of evilophants", &evil_per_check, 0, 256);
    ImGui::End();

    DrawFPS(10, 10);
    DrawText(TextFormat("x: %g, y: %g", player.get<Position>()->pos.x,
                        player.get<Position>()->pos.y),
             100, 10, 20, GRAY);
    // DrawText(TextFormat("vel.x: %g, vel.y: %g",
    // player.get<Velocity>()->vel.x, player.get<Velocity>()->vel.y), 100, 35,
    // 20, GRAY);
    DrawText(TextFormat("enemies: %u", total), 100, 35, 20, GRAY);

    rlImGuiEnd();
#endif

    EndDrawing();
  }

  printf("\nCleaning up...\n");

  // cleanup textures
  for (auto it : TextureLibrary) {
    printf("Unloading texture @%s\n", it.first);
    UnloadTexture(it.second);
  }

#ifdef DEBUG_UI
  rlImGuiShutdown();
#endif

  CloseWindow();

  return 0;
}
