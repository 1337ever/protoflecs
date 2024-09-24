#include <algorithm>
#include <stdio.h>
#include <string>

#include <flecs.h>
#include <raylib.h>

#define WIN_WIDTH 1920
#define WIN_HEIGHT 1020

struct Position {
	float x, y;
};

struct Velocity {
	float x, y;
};

struct ecsColor {
	Color color;
};

struct Paddle { };

int main() {
	flecs::world ecs;

	ecs.set_threads(8);
	ecs.system<Position, const Velocity>()
	   .multi_threaded()
		.each([](Position& p, const Velocity& v) {
			p.x += v.x;
			p.y += v.y;

			if (p.y > WIN_HEIGHT) {
				p.y = 0;
			}
			if (p.y < 0) {
				p.y = WIN_HEIGHT;
			}

			if (p.x > WIN_WIDTH) {
				p.x = 0;
			}
			if (p.x < 0) {
				p.x = WIN_WIDTH;
			}


		});

	ecs.system<Velocity, Paddle>()
		.each([](Velocity& v, const Paddle& p) {
			if (IsKeyDown(KEY_D)) {
				v.x += 0.1;
			}
			if (IsKeyDown(KEY_A)) {
				v.x -= 0.1;
			}
			if (IsKeyDown(KEY_S)) {
				v.y += 0.1;
			}
			if (IsKeyDown(KEY_W)) {
				v.y -= 0.1;
			}
		});

	ecs.system<Velocity>()
		.each([](Velocity& v) {
			v.x = std::clamp(v.x, -50.0f, 50.0f);
		});
	/*
	ecs.entity()
		.set<ecsColor>({MAGENTA})
		.set<Position>({10, 20})
		.set<Velocity>({1, 2});

	ecs.entity()
		.set<ecsColor>({MAROON})
		.set<Position>({10, 20})
		.set<Velocity>({3, 4});

	ecs.entity()
		.set<ecsColor>({GREEN})
		.set<Position>({100, 100})
		.set<Velocity>({1, 0});

	ecs.entity()
		.set<ecsColor>({BLUE})
		.set<Position>({50, 50})
		.set<Velocity>({0, 0})
		.add<Paddle>();
		*/

	flecs::query<const Position, const ecsColor> q = ecs.query<const Position, const ecsColor>();



	//SetTargetFPS(60);
	InitWindow(WIN_WIDTH, WIN_HEIGHT, "raylib test");

	//Shader shader = LoadShader(0, TextFormat("../pixelizer.fs", 330));
	Shader shader_pix = LoadShader(0, TextFormat("../pixelizer.fs", 330));
	//Shader shader_scan = LoadShader(0, TextFormat("../scanlines.fs", 330));

	RenderTexture2D target = LoadRenderTexture(WIN_WIDTH, WIN_HEIGHT);
	while(!WindowShouldClose() && ecs.progress()) {
		BeginTextureMode(target);
		BeginDrawing();

			ClearBackground(RAYWHITE);


			//DrawText("This is a test window", 190, 200, 20, LIGHTGRAY);


			if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
				auto mouse_pos = GetMousePosition();
					ecs.entity()
						.set<ecsColor>({MAGENTA})
						.set<Position>({mouse_pos.x, mouse_pos.y})
						.add<Paddle>()
						.set<Velocity>({0, 0});
			}

			int entity_count = 0;

			q.each([&entity_count](const Position& p, const ecsColor& c) {
				DrawCircleV({p.x, p.y}, 10.0, {c.color.a, static_cast<unsigned char>(c.color.b+static_cast<char>(p.x)), static_cast<unsigned char>(c.color.g+static_cast<char>(p.y)), c.color.r});
				entity_count += 1;
			});
		EndTextureMode();

			BeginShaderMode(shader_pix);
			DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
			EndShaderMode();
			/*
			BeginShaderMode(shader_scan);
			DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
			EndShaderMode();*/

			DrawText(TextFormat("%d drawn entities", entity_count), 250, 10, 40, RED);
			DrawFPS(10, 10);
		EndDrawing();

	}

	UnloadShader(shader_pix);
	//UnloadShader(shader_scan);
	UnloadRenderTexture(target);

	CloseWindow();

	printf("Hello World!\n");
	return 0;
}
