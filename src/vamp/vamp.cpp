#include <algorithm>
#include <cstdio>
#include <vector>
#include <string>
#include <unordered_map>
#include <format>

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#include "subprojects/reasings/src/reasings.h"

#include "subprojects/rlImGui/rlImGui.h"
#include "subprojects/rlImGui/imgui-master/imgui.h"

#include "defines.h"
#include "aabb.cpp"
//#include "textures.cpp"

struct Player {};

struct Enemy {}; //runs toward and attacks player

struct Octophant {};

struct Afraid {}; //runs away from player

struct Ephemeral {
    float buffer = 32;
}; //despawns if out of bounds by buffer degree

struct Health {
    int health = 100;
};

struct Emitter {
    float rate = 0.5;
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

struct Scale {
    float scale = 1.0f;
};

struct Rotation {
    float rot = 0.0f;
};


enum Collision {left, right, top, bottom};

//rect colliders only!!
struct Collider {
    Rectangle box;
    bool solid = true;
    int layer = 1;
    bool colliding = false;
    std::vector<Collision> collisions;
};

//render layer
struct R_Layer {
    int z = 1;
};

struct C_Texture { //raw pointer texture component, deprecated
    Texture2D* tex;
};

struct S_Texture { //lazy loaded texture component
    const char * path;
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

std::vector<Vector2> get_corners(Rectangle rect) {
    std::vector<Vector2> ret = {};
    ret.push_back(Vector2{rect.x, rect.y});
    ret.push_back(Vector2{rect.x+rect.width, rect.y});
    ret.push_back(Vector2{rect.x, rect.y + rect.height});
    ret.push_back(Vector2{rect.x + rect.width + rect.width, rect.y});

    return ret;
}


Vector2* get_corners_ptr(Rectangle rect) {
    static Vector2 ret[4];
    ret[0] = Vector2{rect.x, rect.y};
    ret[1] = Vector2{rect.x+rect.width, rect.y};
    ret[2] = Vector2{rect.x, rect.y + rect.height};
    ret[3] = Vector2{rect.x + rect.width + rect.width, rect.y};
    return ret;
}

std::vector<Collision> check_collision(Rectangle c1, Rectangle c2) {

}


/*inline Rectangle gen_collider_rect(Position& p, Collider& c) {
    return Rectangle{c.box.w+p.pos.x, c.box.x+p.pos.x, c.box.y+p.pos.y, c.box.z+p.pos.y};
}*/

int main() {
    flecs::world ecs;
    ecs.set_threads(4);

    #ifdef ENABLE_REST
    ecs.set<flecs::Rest>({});
    //ecs.import<flecs::monitor>();
    #endif

    //SetTargetFPS(1);
    InitWindow(W_WIDTH, W_HEIGHT, "Octophant Scimitar");

    #ifdef DEBUG_UI
    rlImGuiSetup(true);
    #endif

    //initialize lazy texture library
    std::unordered_map<const char *, Texture2D> TextureLibrary;

    //todo: not have to manually load every texture in code
    /*
    auto t_octophant = LoadTexture("../resources/vamp/char1.png");
    auto t_evilophant = LoadTexture("../resources/vamp/evilophant.png");
    auto t_healophant = LoadTexture("../resources/vamp/healophant.png");

    auto t_heart = LoadTexture("../resources/vamp/heart.png");
    auto t_scimi = LoadTexture("../resources/vamp/scimi2.png");
    */

    /*
    auto sys_lazy_load_textures = ecs.system<const S_Texture>()
        .multi_threaded()
        .each([&TextureLibrary](flecs::entity e1, const S_Texture& st) {

        });
        */

    #ifdef DRAW_COLLIDERS
    auto sys_draw_colliders = ecs.system<const Collider, const Position, const Scale>()
        //.multi_threaded()
        //^^ uncomment for 666 spooky
        .each([](const Collider& c, const Position& p, const Scale& s) {
            float half_width = c.box.width/2;
            float half_height = c.box.height/2;
            //if (c.collisions.size() > 0)
            DrawRectangleLinesEx({c.box.x, c.box.y, c.box.width, c.box.height}, 2.0, c.colliding ? RED : BLUE);
        });
    #endif

    //sync collider positions, annoying this is separate
    auto sys_sync_colliders = ecs.system<const Position, Collider, const Scale>()
        .multi_threaded()
        .each([](const Position& p, Collider& c, const Scale& s) {
            float half_width = (c.box.width)/2;
            float half_height = (c.box.height)/2;
            c.box.x = p.pos.x-half_width;
            c.box.y = p.pos.y-half_height;
        });

    auto sys_vel = ecs.system<Position, const Velocity>()
        .multi_threaded()
        .each([&ecs](Position& p, const Velocity& v) {
            float delta = ecs.delta_time();
            p.pos.x += v.vel.x * delta;
            p.pos.y += v.vel.y * delta;
        });

    auto sys_collision_resolve = ecs.system<Position, Collider>()
        .multi_threaded()
        //.kind(flecs::OnUpdate)
        //this double .each syntax scares me so much i have no idea why.... vvvv
        .each([&ecs](flecs::entity e1, Position& pos1, Collider& col1) {
            //this `.each` doesn't accept `<Position, Collider>`. Oh well
            ecs.each<Collider>([&](flecs::entity e2, Collider& col2) {
                //v prevents double-processing a pair of objects
                if (e1.id() > e2.id()) {
                    if (col1.layer == col2.layer) {
                        if (CheckCollisionRecs(col1.box, col2.box)) {
                            //printf("collide");
                            if (col1.solid && col2.solid) {
                                //hacky inefficient
                                ecs.each<Position>([&](flecs::entity e3, Position& pos2) {
                                    if (e3.id() == e2.id()) {
                                        std::vector<Vector2> cornerListA = get_corners(col1.box);
                                        std::vector<Vector2> cornerListB = get_corners(col2.box);

                                        int sharedcornerindex = 0;
                                        for (int i=0; i<4; i++) {
                                            if (cornerListA[i].x==cornerListB[i].x && cornerListA[i].y == cornerListB[i].y) {
                                                sharedcornerindex++;
                                                break;
                                            }
                                        }

                                        int oppositecornerindex = 0;
                                        if(sharedcornerindex==0) oppositecornerindex = 3;
                                        if(sharedcornerindex==3) oppositecornerindex = 0;
                                        if(sharedcornerindex==1) oppositecornerindex = 2;
                                        if(sharedcornerindex==2) oppositecornerindex = 1;

                                        Vector2 v = Vector2{};
                                        v.x = cornerListB[oppositecornerindex].x - cornerListA[oppositecornerindex].x;
                                        v.y = cornerListB[oppositecornerindex].y - cornerListA[oppositecornerindex].y;

                                        float mag = 40;
                                        float delta = ecs.delta_time();

                                        //todo: replace 3 with scale
                                        pos1.pos.x -= ((v.x/3)*mag)*delta;
                                        pos1.pos.y -= ((v.y/3)*mag)*delta;

                                        #ifdef DUAL_RESTITUTION
                                        pos2.pos.x += ((v.x/3)*mag)*delta;
                                        pos2.pos.y += ((v.y/3)*mag)*delta;
                                        #endif
                                        //pos2.pos.x += v.x/3/2;
                                        //pos2.pos.y += v.y/3/2;
                                    }
                                });
                            }

                            //boolean colliding doesn't really work if there's
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

    //apply a cute velocity-driven rotation to the sprite
    auto anim_octophant = ecs.system<const Velocity, Rotation>()
        .multi_threaded()
        .with<Octophant>()
        .each([](const Velocity& v, Rotation& r) {
            float maxrot = 10.0f; //in degrees
            r.rot = Remap(v.vel.x, -v.max, v.max, -maxrot, maxrot);
        });

    auto sys_damp = ecs.system<Velocity, const Damping>()
        .multi_threaded()
        .each([&ecs](Velocity& v, const Damping& d) {
            float damp = d.damp * ecs.delta_time();

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
        //.set<C_Texture>({&t_octophant})
        .set<S_Texture>({"../resources/vamp/char1.png"})
        //.is_a(it_octophant)
        .set<R_Layer>({1})
        //.set<Collider>({{-8, 8, -8, 8}})
        .set<Collider>({{0, 0, 16*3, 16*3}})
        .add<Player>()
        .add<Octophant>();

    auto healophant = ecs.entity()
        .set<Position>({W_WIDTH/2, W_HEIGHT/2})
        .set<Scale>({3})
        .set<Rotation>({0})
        .set<Velocity>({{0, 0}, 350})
        .set<Damping>({0, 5})
        .set<Speed>({200})
        //.set<C_Texture>({&t_healophant})
        .set<S_Texture>({"../resources/vamp/healophant.png"})
        //.is_a(it_octophant)
        .set<R_Layer>({1})
        //.set<Collider>({{-8, 8, -8, 8}})
        .set<Collider>({{0, 0, 16*3, 16*3}})
        .add<Afraid>()
        .add<Ephemeral>()
        .add<Octophant>();

    auto heart = ecs.entity()
        .set<Position>({W_WIDTH*0.5, 32*3})
        .set<Scale>({5})
        .set<Rotation>({0})
        //.set<C_Texture>({&t_heart})
        .set<S_Texture>({"../resources/vamp/heart.png"})
        .set<R_Layer>({0});

    auto scimi = ecs.entity()
        .set<Position>({W_WIDTH/2, W_HEIGHT/2})
        .set<Scale>({3})
        .set<Rotation>({0})
        //.set<C_Texture>({&t_scimi})
        .set<S_Texture>({"../resources/vamp/scimi2.png"})
        .set<Collider>({{0, 0, 16*3, 16*3}, false})
        .set<R_Layer>({0});
    //auto e2 = ecs.entity().is_a(evilophant);
    //e2.set<Position>({500, 500});

    //insert textures into texture library. multithreaded and separated from draw_sorted
    //to run faster than Update and avoid map lookups every time something needs to be drawn.
    //if these two systems are out of sync, then the entity just gets drawn without a sprite
    //("blank") until it is loaded. hence, lazy loading.
    auto sys_lazy_load = ecs.system<const S_Texture>()
        .multi_threaded()
        .each([&TextureLibrary](const S_Texture& st) {
            if (TextureLibrary.find(st.path) == TextureLibrary.end()) {
                printf("lazy loading texture %s\n", st.path);
                auto t_load = LoadTexture(st.path);
                //auto t_load = LoadTexture(TEX(st.path));
                TextureLibrary[st.path] = t_load;
            }
        });

    auto sys_draw_sorted = ecs.system<const Position, const Scale, const Rotation, const S_Texture, const R_Layer>()
        .kind(flecs::OnUpdate)
        .order_by<R_Layer>(layer_compare) //absolute life saver
        .each([&TextureLibrary](const Position& p, const Scale& s, const Rotation& r, const S_Texture& st, const R_Layer& l) {
            Texture2D* tex = &TextureLibrary[st.path];

            int nw = tex->width;
            int nh = tex->height;
            DrawTexturePro(*tex, {0.0f, 0.0f, (float)nw, (float)nh}, {p.pos.x, p.pos.y, nw*s.scale, nh*s.scale}, {(nw*s.scale)/2, (nh*s.scale)/2}, r.rot, WHITE);
        });

    auto sys_move_enemy = ecs.system<const Position, Velocity, const Speed, const Enemy>()
        .multi_threaded()
        .each([&player](const Position& p, Velocity& v, const Speed& s, const Enemy& e) {
            const Vector2 p_pos = player.get<Position>()->pos;
            //these functions look uggo
            Vector2 dir = Vector2Multiply(Vector2Normalize(Vector2Subtract(p_pos, p.pos)), {s.speed, s.speed});
            v.vel = dir;
        });

    auto sys_move_afraid = ecs.system<const Position, Velocity, const Speed, const Afraid>()
        .multi_threaded()
        .each([&player](const Position& p, Velocity& v, const Speed& s, const Afraid& a) {
            const Vector2 p_pos = player.get<Position>()->pos;
            //these functions look uggo
            Vector2 dir = Vector2Multiply(Vector2Normalize(Vector2Subtract(p_pos, p.pos)), {s.speed, s.speed});
            v.vel = Vector2Multiply(dir, Vector2{-1, -1});
        });

    auto sys_despawn_ephemeral = ecs.system<const Position, const Ephemeral>()
        .multi_threaded()
        .each([](flecs::entity e1, const Position& p, const Ephemeral& e) {
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
            //printf("destructing entity %s\n", e1);
        });

    auto sys_character_controller = ecs.system<Velocity, Damping, const Speed, const Player>()
        .multi_threaded()
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

    int count = 0;
    int total = 0;
    int evil_per_check = 1;
    int interval = 5;
    while(!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            //SetTargetFPS(600);


            ecs.progress(GetFrameTime());

            //spawn an evilophant in a random location every 5 seconds
            //todo: make them spawn offscreen
            bool spawn = (int)GetTime() % interval ? false : true;
            if(spawn && count < evil_per_check) {
                ecs.entity()
                        .set<Position>({(float)GetRandomValue(0, 1000), (float)GetRandomValue(0, 1000)})
                        .set<Scale>({3})
                        .set<Rotation>({0.0})
                        .set<Velocity>({{0, 0}, 350})
                        .set<Speed>({200})
                        //.set<C_Texture>({&t_evilophant})
                        .set<S_Texture>({"../resources/vamp/evilophant.png"})
                        //.is_a(it_evilophant)
                        .set<R_Layer>({2})
                        //.set<Collider>({{-8, 8, -8, 8}})
                        .set<Collider>({{0, 0, 16*3, 16*3}})
                        .add<Enemy>()
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
            DrawText(TextFormat("x: %g, y: %g", player.get<Position>()->pos.x, player.get<Position>()->pos.y), 100, 10, 20, GRAY);
            //DrawText(TextFormat("vel.x: %g, vel.y: %g", player.get<Velocity>()->vel.x, player.get<Velocity>()->vel.y), 100, 35, 20, GRAY);
            DrawText(TextFormat("enemies: %u", total), 100, 35, 20, GRAY);

            rlImGuiEnd();
            #endif

        EndDrawing();
    }

    //cleanup textures
    //idk if its okay to try to unload the same pointer a hundred times, it seems to work though
    ecs.each([](C_Texture& t) {
        UnloadTexture(*t.tex);
    });

    #ifdef DEBUG_UI
    rlImGuiShutdown();
    #endif

    CloseWindow();

    return 0;
}
