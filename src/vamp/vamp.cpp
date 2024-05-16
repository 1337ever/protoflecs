#include <algorithm>
#include <vector>

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
    Texture2D* tex;
};

struct I_Texture {
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

//tag component indicating the attached entity has already been sorted into the sprite layering system
struct Sorted {};

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
        .add<Player>();

    auto evilophant = ecs.entity()
        .set<Position>({100, 100})
        .set<Scale>({3})
        .set<Rotation>({0})
        .set<Velocity>({{10, 10}, 350})
        .set<C_Texture>({&t_evilophant})
        //.is_a(it_evilophant)
        .set<R_Layer>({1})
        .add<Enemy>();

    //auto e2 = ecs.entity().is_a(evilophant);
    //e2.set<Position>({500, 500});
    
    //awesome array of vectors, yeah i manage memory, yeah i go fastish
    std::vector<flecs::entity> drawables[MAX_RENDER_LAYERS];
    //getting a pointer == optional component?
    auto sys_sort_drawables = ecs.system<const Position, const Scale, const Rotation, C_Texture, const R_Layer>()
        .kind(flecs::OnUpdate)
        .iter([&drawables](flecs::iter& it, const Position *p, const Scale *s, const Rotation *r, C_Texture *t, const R_Layer *l) {
            //DrawTextureEx(t.tex, p.pos, r.rot, s.scale, WHITE);
            //flecs::entity drawables[it.count()];


            //for (auto i : it) {
            for (size_t i = 0; i < it.count(); i ++) {
                //if (drawables[l[i].layer] != it.entity(i))
                printf("inserting layer %d\t", l[i].layer);
                drawables[l[i].layer].push_back(it.entity(i));
            }
            
            printf("\n");
            for (int v = MAX_RENDER_LAYERS-1; v >=  0; v--) {
                printf("%d\n", v);
                //printf("drawing layer: %d\n", v);
                for (int d = 0; d < drawables[v].size(); d++) {
                    flecs::entity e =  drawables[v][d];
                    printf("drawing entity: %d on layer %d\n", d, v);
                    auto tex = e.get_ref<C_Texture>()->tex;
                    auto pos = e.get_ref<Position>()->pos;
                    auto rot = e.get_ref<Rotation>()->rot;
                    auto scale = e.get_ref<Scale>()->scale; 

                    DrawTextureEx(*tex, pos, rot, scale, WHITE);
                } 
            }
        });
/*
    auto sys_draw_object = ecs.system<const Position, const Scale, const Rotation, C_Texture, const R_Layer>()
        .kind(flecs::OnUpdate)
        .iter([&drawables](flecs::iter& it, const Position *p, const Scale *s, const Rotation *r, C_Texture *t, const R_Layer *l) {
            //DrawTextureEx(t.tex, p.pos, r.rot, s.scale, WHITE);
            //flecs::entity drawables[it.count()];

            for (int v = MAX_RENDER_LAYERS-1; v >=  0; v--) {
                //printf("%d\n", v);
                //printf("drawing layer: %d\n", v);
                for (int d = 0; d < drawables[v].size(); d++) {
                    flecs::entity e =  drawables[v][d];
                    //printf("drawing entity: %d on layer %d\n", d, v);
                    auto tex = e.get_ref<C_Texture>()->tex;
                    auto pos = e.get_ref<Position>()->pos;
                    auto rot = e.get_ref<Rotation>()->rot;
                    auto scale = e.get_ref<Scale>()->scale; 

                    DrawTextureEx(*tex, pos, rot, scale, WHITE);
                } 
            }
        });*/

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
        //EndTextureMode();

        //DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
    }

    //cleanup textures
    ecs.each([](C_Texture& t) {
        UnloadTexture(*t.tex);
    });

    CloseWindow();

    return 0;
}