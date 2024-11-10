#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "imgui.h"
#include "rlImGui.h"

#include "resource_dir.h"

#define	STB_DS_IMPLEMENTATION
#include "stb_ds.h"

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

typedef enum
{
	BOL_EMPTY,

	BOL_NA,
	BOL_TI,
	BOL_TA,
	BOL_TIN,

	BOL_KE,
	BOL_GE,
	BOL_DHA,
	BOL_DHIN,
	BOL_DHET,

	BOL_KRE,
	BOL_TET,

	BOL_COUNT
} Bol;

const char* BolStr[BOL_COUNT] = 
{
	"-",

	"NA",
	"TI",
	"TA",
	"TIN",

	"KE",
	"GE",
	"DHA",
	"DHIN",
	"DHET",

	"BOL_KRE",
	"BOL_TET"
};

typedef struct BolViz
{
	Bol bol;
	Rectangle rect;
} BolViz;

typedef struct Composition
{
	BolViz* pattern;
	Vector2 origin;
	int hovered;
} Composition;

typedef struct Assets
{
	Texture wabbit;
} Assets;

typedef struct Mouse
{
	Vector2 screenPos;
	Vector2 worldPos;
} Mouse;

typedef struct State
{
	Camera2D camera;
	Mouse mouse;
	Composition composition;
} State;

// ----------------------------------------------------------------------------
// globals / constants
// ----------------------------------------------------------------------------

// https://coolors.co/palette/606c38-283618-fefae0-dda15e-bc6c25
static const Color BOL_COLOR_BACKGROUND = { 40, 54, 24, 255 }; // pakistan green
static const Color BOL_COLOR_BORDER = { 96, 108, 56, 255 }; // dark moss green
static const Color BOL_COLOR_TEXT = { 254, 250, 224, 255 }; // cornsilk
static const Color BOL_COLOR_BACKGROUND_HOVER = { 188, 108, 37, 255 }; // earth yellow
static const Color BOL_COLOR_BORDER_HOVER = { 221, 161, 94, 255 }; // tiger's eye
static const Color BOL_COLOR_TEXT_HOVER = { 255, 215, 0, 255 }; // gold

static Assets assets = {0};
static State state = {0};

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

void initialize()
{
	// init raylib
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "Hello Raylib");
	SetTargetFPS(144);

	// init raylib-imgui
	rlImGuiSetup(true);

	// Utility function from resource_dir.h to find the resources folder
	// and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	// init assets
	assets.wabbit = LoadTexture("wabbit_alpha.png");

	// init state
	state.mouse = { 0 };
	state.camera = { 0 };
	state.camera.zoom = 1.0f;

	// init composition
	const int numBols = 16;
	state.composition = { 0 };
	state.composition.origin = { 200, 250 };
	arrinsn(state.composition.pattern, 0, numBols);

	// initialize bols for tintal theka
	BolViz* pattern = state.composition.pattern;
	pattern[0] = { BOL_DHA, {0} };
	pattern[1] = { BOL_DHIN, {0} };
	pattern[2] = { BOL_DHIN, {0} };
	pattern[3] = { BOL_DHA, {0} };
	pattern[4] = { BOL_DHA, {0} };
	pattern[5] = { BOL_DHIN, {0} };
	pattern[6] = { BOL_DHIN, {0} };
	pattern[7] = { BOL_DHA, {0} };
	pattern[8] = { BOL_DHA, {0} };
	pattern[9] = { BOL_TIN, {0} };
	pattern[10] = { BOL_TIN, {0} };
	pattern[11] = { BOL_TA, {0} };
	pattern[12] = { BOL_TA, {0} };
	pattern[13] = { BOL_DHIN, {0} };
	pattern[14] = { BOL_DHIN, {0} };
	pattern[15] = { BOL_DHA, {0} };

	// layout the bols in tintal
	const int tintal = 16;

	const int taal = tintal;
	const int numBolsPerLine = taal / 2;
	const int numCycles = numBols / taal;

	const float margin = 5;
	const float cycleGap = 50;
	const float width = 100 - 2 * margin;
	const float height = 50 - 2 * margin;

	float x = margin;
	float y = margin;
	for (int i = 0, l = 0, c = 0; i < numBols; i++)
	{
		if (i != 0)
		{
			// move to next line
			if (i % numBolsPerLine == 0)
			{
				l++;
				x = margin;
				y += height + 2 * margin;
			}

			// move to next cycle
			if (i % taal == 0)
			{
				c++;
				i = 0;
				x = margin;
				y += cycleGap + margin;
			}
		}

		pattern[i].rect = { x, y, width, height };

		// move to next bol
		x += width + 2 * margin;
	}
}

void shutdown()
{
	// release assets
	UnloadTexture(assets.wabbit);

	// release composition
	arrfree(state.composition.pattern);

	// shutdown raylib-imgui
	rlImGuiShutdown();

	// shutdown raylib
	CloseWindow();
}

void update()
{
	state.mouse.screenPos = GetMousePosition();
	state.mouse.worldPos = GetScreenToWorld2D(state.mouse.screenPos, state.camera);

	// pan
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
		state.camera.offset = state.mouse.screenPos;
		state.camera.target = state.mouse.worldPos;

		float scaleFactor = 1.0f + (0.25f * fabsf(wheel));
		if (wheel < 0) scaleFactor = 1.0f / scaleFactor;
		state.camera.zoom = Clamp(state.camera.zoom * scaleFactor, 0.125f, 64.0f);
	}

	// composition bol hover
	state.composition.hovered = -1;
	const Vector2 origin = state.composition.origin;
	const BolViz* pattern = state.composition.pattern;
	const int numBols = arrlen(pattern);
	for (int i = 0; i < numBols; i++)
	{
		BolViz bol = pattern[i];

		float left   = origin.x + bol.rect.x;
		float top    = origin.y + bol.rect.y;
		float right  = left + bol.rect.width;
		float bottom = top + bol.rect.height;

		Vector2 mouse = state.mouse.worldPos;
		bool contains = (mouse.x >= left) && (mouse.x <= right)
			         && (mouse.y >= top) && (mouse.y <= bottom);
		if (contains)
		{
			state.composition.hovered = i;
			break;
		}
	}
}

void drawGrid(int slices, int spacing)
{
	rlPushMatrix();
	rlTranslatef(0, 25 * 50, 0);
	rlRotatef(90, 1, 0, 0);

	Color zero = { 0, 160, 0, 255 };
	Color normal = { 50, 50, 50, 255 };
	int halfSlices = slices/2;

	rlBegin(RL_LINES);
	for (int i = -halfSlices; i <= halfSlices; i++)
	{
		Color color = (i == 0) ? zero : normal;
		rlColor4ub(color.r, color.g, color.b, color.a);

		rlVertex3f((float)i*spacing, 0.0f, (float)-halfSlices*spacing);
		rlVertex3f((float)i*spacing, 0.0f, (float)halfSlices*spacing);

		rlVertex3f((float)-halfSlices*spacing, 0.0f, (float)i*spacing);
		rlVertex3f((float)halfSlices*spacing, 0.0f, (float)i*spacing);
	}
	rlColor4f(1, 1, 1, 1);
	rlEnd();

	rlPopMatrix();
}

void drawComposition(const Composition* composition, float originX, float originY)
{
	const float fontSize = 20;
	const float lineThick = 2;
	const float roundness = 4;
	const int segments = 8;

	const int numBols = arrlen(composition->pattern);
	for (int i = 0; i < numBols; i++)
	{
		BolViz bol = composition->pattern[i];
		Rectangle rect = {
			originX + bol.rect.x,
			originY + bol.rect.y,
			bol.rect.width,
			bol.rect.height
		};

		const bool isHovered = (i == composition->hovered);
		Color backCol = isHovered ? BOL_COLOR_BACKGROUND_HOVER : BOL_COLOR_BACKGROUND;
		Color bordCol = isHovered ? BOL_COLOR_BORDER_HOVER : BOL_COLOR_BORDER;
		Color textCol = isHovered ? BOL_COLOR_TEXT_HOVER : BOL_COLOR_TEXT;

		DrawRectangleRounded(rect, roundness, segments, backCol);
		DrawRectangleRoundedLinesEx(rect, roundness, segments, lineThick, bordCol);

		const char* text = BolStr[bol.bol];
		const float textWidth = MeasureText(text, fontSize);
		const float offsetX = (rect.width - textWidth) / 2.0f;
		const float offsetY = (rect.height - fontSize) / 2.0f;
		DrawText(text, rect.x + offsetX, rect.y + offsetY, fontSize, textCol);
	}
}

void drawUI()
{
	static bool open = true;

	rlImGuiBegin();
	ImGui::ShowDemoWindow(&open);
	rlImGuiEnd();
}

void draw()
{
	BeginDrawing();

	static const Color bg = { 30, 30, 30, 255 };
	ClearBackground(bg);

	BeginMode2D(state.camera);
	{
		drawGrid(100, 50);
		drawComposition(&state.composition, 200, 250);

		DrawText("Hello Raylib", 200,200,20,WHITE);
		DrawTexture(assets.wabbit, 400, 200, WHITE);
	}
	EndMode2D();

	drawUI();

	EndDrawing();
}

// ----------------------------------------------------------------------------
// entry / main loop
// ----------------------------------------------------------------------------

int main ()
{
	initialize();
	while (!WindowShouldClose())
	{
		update();
		draw();
	}
	shutdown();
	return 0;
}

