#include <algorithm>
#include <vector>

#include <flecs.h>
#include <raylib.h>
#include <subprojects/reasings/src/reasings.h>

#include "defines.h"
#include "aabb.cpp"
//#include "textures.cpp"

struct Player {};

struct Enemy {};

struct Octophant {};

struct Health {
    int health = 100;
};

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

//velocity damping, for simulating ie friction
struct Damping {
    float damp; //current damping
    float def; //default damping
};

struct C_Color {
    Color color = RED;
};

struct C_Texture {
    Texture2D* tex;
};

struct I_Texture {
    Texture2D tex;
};

struct Scale {
    float scale = 1.0f;
};

struct Rotation {
    float rot = 0.0f;
};

//square colliders only!!
struct Collider {
    Vector4 box;
};

//collision layer
struct Col_Layer {
    int z = 1;
};

//render layer
struct R_Layer {
    int z = 1;
};

static inline int layer_compare(flecs::entity_t e1, const R_Layer* l1, flecs::entity_t e2, const R_Layer* l2) {
  if (l1->z < l2->z) {
    return 1;
  } else if (l1->z > l2->z) {
    return -1;
  } else {
    return 0;
  }
}

int main() {
    flecs::world ecs;
    ecs.set_threads(4);
    
    #ifdef ENABLE_REST
    ecs.set<flecs::Rest>({});
    ecs.import<flecs::monitor>();
    #endif

    //SetTargetFPS(1);
    InitWindow(W_WIDTH, W_HEIGHT, "Octophant Scimitar");

    auto sys_vel = ecs.system<Position, const Velocity>()
        .multi_threaded()
        .each([](Position& p, const Velocity& v) {
            float delta = GetFrameTime();
            p.pos.x += v.vel.x * delta;
            p.pos.y += v.vel.y * delta;
        });

    auto anim_octophant = ecs.system<const Velocity, Rotation>()
        .with<Octophant>()
        .each([](const Velocity& v, Rotation& r) {
            float maxrot = 10.0f; //in degrees
            r.rot = Remap(v.vel.x, -v.max, v.max, -maxrot, maxrot);
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

    auto t_octophant = LoadTexture("../resources/vamp/char1.png");
    auto t_evilophant = LoadTexture("../resources/vamp/evilophant.png");

    //auto it_octophant = ecs.entity().set<I_Texture>({LoadTexture("../resources/vamp/char1.png")});
    //auto it_evilophant = ecs.entity().set<I_Texture>({LoadTexture("../resources/vamp/evilophant.png")});

    auto player = ecs.entity()
        .set<Position>({W_WIDTH/2, W_HEIGHT/2})
        .set<Scale>({3})
        .set<Rotation>({0})
        .set<Velocity>({{0, 0}, 350})
        .set<Damping>({0, 5})
        .set<Speed>({5000})
        .set<C_Texture>({&t_octophant})
        //.is_a(it_octophant)
        .set<R_Layer>({0})
        .add<Player>()
        .add<Octophant>();

    auto evilophant = ecs.entity()
        .set<Position>({100, 100})
        .set<Scale>({3})
        .set<Rotation>({0.0})
        .set<Velocity>({{20, 10}, 350})
        .set<C_Texture>({&t_evilophant})
        //.is_a(it_evilophant)
        .set<R_Layer>({1})
        .add<Enemy>()
        .add<Octophant>();

    //auto e2 = ecs.entity().is_a(evilophant);
    //e2.set<Position>({500, 500});
    
    //getting a pointer == optional component?
    auto sys_draw_sorted = ecs.system<const Position, const Scale, const Rotation, const C_Texture, const R_Layer>()
        .kind(flecs::OnUpdate)
        .order_by<R_Layer>(layer_compare) //absolute life saver
        .each([](const Position& p, const Scale& s, const Rotation& r, const C_Texture& t, const R_Layer& l) {
            //DrawTextureEx(*t.tex, p.pos, r.rot, s.scale, WHITE);

            int nw = t.tex->width;
            int nh = t.tex->height;
            DrawTexturePro(*t.tex, {0.0f, 0.0f, (float)nw, (float)nh}, {p.pos.x, p.pos.y, nw*s.scale, nh*s.scale}, {(nw*s.scale)/2, (nh*s.scale)/2}, r.rot, WHITE);
            //use drawtexturepro to shift origin
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

    //RenderTexture2D target = LoadRenderTexture(W_WIDTH, W_HEIGHT);
    while(!WindowShouldClose()) {
       // BeginTextureMode(target);
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
        UnloadTexture(*t.tex);
    });

    CloseWindow();

    return 0;
}