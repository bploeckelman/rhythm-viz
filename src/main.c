#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "imgui.h"
#include "rlImGui.h"

#include "resource_dir.h"

struct Assets
{
	Texture wabbit;
};
typedef struct Assets Assets;

struct State
{
	Camera2D camera;
};
typedef struct State State;

Assets assets = {0};
State state = {0};

void update();
void draw();
void drawUI();

int main ()
{
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);

	InitWindow(1280, 800, "Hello Raylib");
	SetTargetFPS(144);

	rlImGuiSetup(true);

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	assets.wabbit = LoadTexture("wabbit_alpha.png");

	state.camera.zoom = 1.0f;
	
	while (!WindowShouldClose())
	{
		update();
		draw();
	}

	UnloadTexture(assets.wabbit);

	rlImGuiShutdown();
	CloseWindow();
	return 0;
}

void update()
{
	// translate
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		Vector2 delta = GetMouseDelta();
		delta = Vector2Scale(delta, -1.0f / state.camera.zoom);
		state.camera.target = Vector2Add(state.camera.target, delta);
	}

	// zoom
	float wheel = GetMouseWheelMove();
	if (wheel != 0)
	{
		Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), state.camera);
		state.camera.offset = GetMousePosition();
		state.camera.target = mouseWorldPos;

		float scaleFactor = 1.0f + (0.25f * fabsf(wheel));
		if (wheel < 0) scaleFactor = 1.0f / scaleFactor;
		state.camera.zoom = Clamp(state.camera.zoom * scaleFactor, 0.125f, 64.0f);
	}
}

void draw()
{
	BeginDrawing();

	static const Color bg = { 30, 30, 30, 255 };
	ClearBackground(bg);

	BeginMode2D(state.camera);
	{
		// background grid
		rlPushMatrix();
		rlTranslatef(0, 25 * 50, 0);
		rlRotatef(90, 1, 0, 0);
		{
			DrawGrid(100, 50);
		}
		rlPopMatrix();

		DrawText("Hello Raylib", 200,200,20,WHITE);
		DrawTexture(assets.wabbit, 400, 200, WHITE);
	}
	EndMode2D();

	drawUI();

	EndDrawing();
}

void drawUI()
{
	static bool open = true;

	rlImGuiBegin();
	ImGui::ShowDemoWindow(&open);
	rlImGuiEnd();
}
