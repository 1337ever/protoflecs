#include <algorithm>

#include <flecs.h>
#include <raylib.h>

#include "defines.h"
#include "aabb.cpp"
//#include "textures.cpp"

struct Player {};

struct Enemy {};

struct Health {
    int health;
};

struct Speed {
    float speed;
};

struct Position {
	Vector2 pos;
};

struct Velocity {
	Vector2 vel;
    float max;
};

//velocity damping, for simulating ie friction
struct Damping {
    float damp; //current damping
    float def; //default damping
};

struct C_Color {
    Color color;
};

struct C_Texture {
    Texture2D tex;
};

struct Scale {
    float scale;
};

struct Rotation {
    float rot;
};

//square colliders only!!
struct Collider {
    Vector4 box;
};

//collision layer
struct Col_Layer {
    int layer;
};

//render layer
struct R_Layer {
    int layer;
};

int main() {
    flecs::world ecs;

    //SetTargetFPS(60);
    InitWindow(W_WIDTH, W_HEIGHT, "Octophant Scimitar");

    auto sys_vel = ecs.system<Position, const Velocity>()
        .each([](Position& p, const Velocity& v) {
            float delta = GetFrameTime();
            p.pos.x += v.vel.x * delta;
            p.pos.y += v.vel.y * delta;
        });

    auto sys_damp = ecs.system<Velocity, const Damping>()
        .each([](Velocity& v, const Damping& d) {
            float delta = GetFrameTime();
            float damp = d.damp * delta;

            v.vel.x -= damp * v.vel.x;
            v.vel.y -= damp * v.vel.y;

            v.vel.x = std::clamp(v.vel.x, -v.max, v.max);
            v.vel.y = std::clamp(v.vel.y, -v.max, v.max);
        });

    auto player = ecs.entity()
        .set<Position>({W_WIDTH/2, W_HEIGHT/2})
        .set<Scale>({3})
        .set<Rotation>({0})
        .set<Velocity>({{0, 0}, 350})
        .set<Damping>({0, 5})
        .set<Speed>({5000})
        .set<C_Texture>({LoadTexture("../resources/vamp/char1.png")})
        .set<R_Layer>({0})
        .add<Player>();

    auto evilophant = ecs.entity()
        .set<Position>({100, 100})
        .set<Scale>({3})
        .set<Rotation>({0})
        .set<Velocity>({{10, 10}, 350})
        .set<C_Texture>({LoadTexture("../resources/vamp/evilophant.png")})
        .set<R_Layer>({0})
        .add<Enemy>();

    auto sys_draw_object = ecs.system<const Position, const Scale, const Rotation, const C_Texture>()
        .each([](const Position& p, const Scale& s, const Rotation& r, const C_Texture& t) {
            DrawTextureEx(t.tex, p.pos, r.rot, s.scale, WHITE);
        });

    auto sys_character_controller = ecs.system<Velocity, Damping, const Speed, const Player>()
        .each([](Velocity& v, Damping& d, const Speed& s, const Player& p) {
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

                //refine so that each axis is minspeeded separately (if one axis is no longer moving)
                //also each axis needs to be damped separately for similar reasons
                float minspeed = CHARACTER_MIN_SPEED;
                if (v.vel.x < minspeed && !(v.vel.x < -minspeed)) {
                    v.vel.x = 0;
                }
                if (v.vel.y < minspeed && !(v.vel.y < -minspeed)) {
                    v.vel.y = 0;
                }
            }
        });

    while(!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            ecs.progress(GetFrameTime());


            DrawFPS(10, 10);
            DrawText(TextFormat("x: %g, y: %g", player.get<Position>()->pos.x, player.get<Position>()->pos.y), 100, 10, 20, GRAY);
            DrawText(TextFormat("vel.x: %g, vel.y: %g", player.get<Velocity>()->vel.x, player.get<Velocity>()->vel.y), 100, 35, 20, GRAY);
        EndDrawing();
        
    }

    //cleanup textures
    ecs.each([](C_Texture& t) {
        UnloadTexture(t.tex);
    });

    CloseWindow();

    return 0;
}