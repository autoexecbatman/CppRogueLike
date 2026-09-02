// file: Renderer.cpp
#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <string>

#include <raylib.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <rlgl.h>
#endif

#include "../Systems/TileConfig.h"
#include "Renderer.h"

// Local color constants (raylib macros are #undef'd in Renderer.h)
constexpr Color RL_WHITE = { 255, 255, 255, 255 };
constexpr Color RL_BLACK = { 0, 0, 0, 255 };
constexpr Color RL_RED = { 230, 41, 55, 255 };
constexpr Color RL_GREEN = { 0, 228, 48, 255 };
constexpr Color RL_BLUE = { 0, 121, 241, 255 };
constexpr Color RL_YELLOW = { 253, 249, 0, 255 };
constexpr Color RL_MAGENTA = { 255, 0, 255, 255 };

constexpr double animInterval = 0.5;

constexpr std::size_t sheet_idx(TileSheet s) noexcept
{
	return static_cast<std::size_t>(s);
}

// DawnLike uses magenta (255,0,255) as the transparency key.
// Load a PNG, replace magenta pixels with alpha=0, return as Texture2D.
Texture2D load_dawnlike_texture(std::string_view path)
{
	std::string path_str(path);
	Image image = LoadImage(path_str.c_str());
	if (image.data == nullptr)
	{
		return Texture2D{};
	}
	ImageColorReplace(&image, RL_MAGENTA, Color{ 0, 0, 0, 0 });
	Texture2D texture = LoadTextureFromImage(image);
	SetTextureFilter(texture, TEXTURE_FILTER_POINT);
	UnloadImage(image);
	return texture;
}

void Renderer::init()
{
#ifdef EMSCRIPTEN
	// Query the actual browser window size and snap to tile grid so no partial
	// tiles appear at the edges.
	{
		int rawWidth = EM_ASM_INT({ return window.innerWidth; });
		int rawHeight = EM_ASM_INT({ return window.innerHeight; });
		screenWidth = (rawWidth / tileSize) * tileSize;
		screenHeight = (rawHeight / tileSize) * tileSize;
	}
	InitWindow(screenWidth, screenHeight, "C++RogueLike");
	// Browser rAF drives timing -- disable raylib's internal WaitTime/emscripten_sleep.
	SetTargetFPS(0);
#else
	// Create initial window to query monitor
	InitWindow(800, 600, "C++RogueLike");
	SetTargetFPS(60);
	SetExitKey(0);

	int monitor = GetCurrentMonitor();
	screenWidth = GetMonitorWidth(monitor);
	screenHeight = GetMonitorHeight(monitor);

	// Go fullscreen at monitor resolution
	SetWindowSize(screenWidth, screenHeight);
	SetWindowPosition(0, 0);
	// ToggleFullscreen();  // DISABLED FOR DEBUGGING
#endif

	// Viewport in tile units
	viewportCols = screenWidth / tileSize;
	viewportRows = screenHeight / tileSize;

	// Font scaled proportionally to tile size
	fontSize = tileSize * 3 / 4;

	init_color_pairs();

	lightMask = LoadRenderTexture(screenWidth, screenHeight);
	lightMaskLoaded = (lightMask.id > 0);

	initialized = true;
}

void Renderer::update_viewport()
{
	screenWidth = GetScreenWidth();
	screenHeight = GetScreenHeight();
	viewportCols = screenWidth / tileSize;
	viewportRows = screenHeight / tileSize;
}

void Renderer::shutdown()
{
	for (SpriteSheet& sheet : sheets)
	{
		if (sheet.loaded)
		{
			UnloadTexture(sheet.frame0);
			if (sheet.animated)
			{
				UnloadTexture(sheet.frame1);
			}
			sheet.loaded = false;
		}
	}
	sheetsLoaded = false;

	if (fontLoaded)
	{
		UnloadFont(gameFont);
		fontLoaded = false;
	}

	if (lightMaskLoaded)
	{
		UnloadRenderTexture(lightMask);
		lightMaskLoaded = false;
	}

	if (initialized)
	{
		CloseWindow();
		initialized = false;
	}
}

void Renderer::load_sheet(TileSheet id, std::string_view name, std::string_view path0, std::string_view path1, int cellSize)
{
	auto& s = sheets[sheet_idx(id)];
	s.name = name;
	s.cellSize = cellSize;
	s.frame0 = load_dawnlike_texture(path0);
	s.frame1 = load_dawnlike_texture(path1);
	s.tilesPerRow = s.frame0.width / cellSize;
	s.tilesPerCol = s.frame0.height / cellSize;
	s.animated = (s.frame1.id > 0);
	s.loaded = (s.frame0.id > 0);
}

void Renderer::load_sheet_static(TileSheet id, std::string_view name, std::string_view path, int cellSize)
{
	auto& s = sheets[sheet_idx(id)];
	s.name = name;
	s.cellSize = cellSize;
	s.frame0 = load_dawnlike_texture(path);
	s.frame1 = s.frame0;
	s.tilesPerRow = s.frame0.width / cellSize;
	s.tilesPerCol = s.frame0.height / cellSize;
	s.animated = false;
	s.loaded = (s.frame0.id > 0);
}

int Renderer::get_sheet_cols(TileSheet sheet) const
{
	if (sheet_idx(sheet) >= sheets.size())
	{
		return 0;
	}
	return sheets[sheet_idx(sheet)].tilesPerRow;
}

int Renderer::get_sheet_rows(TileSheet sheet) const
{
	if (sheet_idx(sheet) >= sheets.size())
	{
		return 0;
	}
	return sheets[sheet_idx(sheet)].tilesPerCol;
}

bool Renderer::sheet_is_loaded(TileSheet sheet) const
{
	if (sheet_idx(sheet) >= sheets.size())
	{
		return false;
	}
	return sheets[sheet_idx(sheet)].loaded;
}

std::string_view Renderer::get_sheet_name(TileSheet sheet) const
{
	if (sheet_idx(sheet) >= sheets.size())
	{
		return {};
	}
	return sheets[sheet_idx(sheet)].name;
}

int Renderer::get_loaded_sheet_count() const
{
	int count = 0;
	for (const auto& s : sheets)
	{
		if (s.loaded)
		{
			++count;
		}
	}
	return count;
}

void Renderer::load_dawnlike(std::string_view basePath)
{
	std::string base(basePath);
	if (!base.empty() && base.back() != '/')
	{
		base += '/';
	}

	auto load_animated = [&](TileSheet id, std::string_view name, const char* dir, const char* file, int cellSize)
	{
		std::string p0 = std::format("{}{}{}0.png", base, dir, file);
		std::string p1 = std::format("{}{}{}1.png", base, dir, file);
		load_sheet(id, name, p0, p1, cellSize);
	};

	auto load_static = [&](TileSheet id, std::string_view name, const char* dir, const char* file, int cellSize)
	{
		std::string p = std::format("{}{}{}.png", base, dir, file);
		load_sheet_static(id, name, p, cellSize);
	};

	// Objects
	load_static(TileSheet::SHEET_FLOOR, "Floor", "Objects/", "Floor", 16);
	load_static(TileSheet::SHEET_WALL, "Wall", "Objects/", "Wall", 16);
	load_static(TileSheet::SHEET_DOOR0, "Door0", "Objects/", "Door0", 16);
	load_animated(TileSheet::SHEET_DECOR0, "Decor0", "Objects/", "Decor", 16);
	load_animated(TileSheet::SHEET_EFFECT0, "Effect0", "Objects/", "Effect", 16);
	load_static(TileSheet::SHEET_TILE, "Tile", "Objects/", "Tile", 16);
	load_animated(TileSheet::SHEET_PIT0, "Pit0", "Objects/", "Pit", 16);
	load_animated(TileSheet::SHEET_GUI0, "GUI0", "GUI/", "GUI", 16);

	// Characters (all animated with 0/1 pairs)
	load_animated(TileSheet::SHEET_PLAYER0, "Player0", "Characters/", "Player", 16);
	load_animated(TileSheet::SHEET_HUMANOID0, "Humanoid0", "Characters/", "Humanoid", 16);
	load_animated(TileSheet::SHEET_REPTILE0, "Reptile0", "Characters/", "Reptile", 16);
	load_animated(TileSheet::SHEET_PEST0, "Pest0", "Characters/", "Pest", 16);
	load_animated(TileSheet::SHEET_DOG0, "Dog0", "Characters/", "Dog", 16);
	load_animated(TileSheet::SHEET_AVIAN0, "Avian0", "Characters/", "Avian", 16);
	load_animated(TileSheet::SHEET_UNDEAD0, "Undead0", "Characters/", "Undead", 16);
	load_animated(TileSheet::SHEET_QUADRAPED0, "Quadraped0", "Characters/", "Quadraped", 16);
	load_animated(TileSheet::SHEET_DEMON0, "Demon0", "Characters/", "Demon", 16);
	load_animated(TileSheet::SHEET_MISC0, "Misc0", "Characters/", "Misc", 16);

	// Items (static -- no animation frames)
	load_static(TileSheet::SHEET_POTION, "Potion", "Items/", "Potion", 16);
	load_static(TileSheet::SHEET_SCROLL, "Scroll", "Items/", "Scroll", 16);
	load_static(TileSheet::SHEET_SHORT_WEP, "ShortWep", "Items/", "ShortWep", 16);
	load_static(TileSheet::SHEET_MED_WEP, "MedWep", "Items/", "MedWep", 16);
	load_static(TileSheet::SHEET_LONG_WEP, "LongWep", "Items/", "LongWep", 16);
	load_static(TileSheet::SHEET_ARMOR, "Armor", "Items/", "Armor", 16);
	load_static(TileSheet::SHEET_SHIELD, "Shield", "Items/", "Shield", 16);
	load_static(TileSheet::SHEET_HAT, "Hat", "Items/", "Hat", 16);
	load_static(TileSheet::SHEET_RING, "Ring", "Items/", "Ring", 16);
	load_static(TileSheet::SHEET_AMULET_ITEM, "Amulet", "Items/", "Amulet", 16);
	load_static(TileSheet::SHEET_FOOD, "Food", "Items/", "Food", 16);
	load_static(TileSheet::SHEET_FLESH, "Flesh", "Items/", "Flesh", 16);
	load_static(TileSheet::SHEET_MONEY, "Money", "Items/", "Money", 16);

	// Previously unloaded item sheets
	load_static(TileSheet::SHEET_AMMO, "Ammo", "Items/", "Ammo", 16);
	load_static(TileSheet::SHEET_WAND, "Wand", "Items/", "Wand", 16);
	load_static(TileSheet::SHEET_BOOK, "Book", "Items/", "Book", 16);
	load_static(TileSheet::SHEET_BOOT, "Boot", "Items/", "Boot", 16);
	load_static(TileSheet::SHEET_GLOVE, "Glove", "Items/", "Glove", 16);
	load_static(TileSheet::SHEET_KEY, "Key", "Items/", "Key", 16);
	load_static(TileSheet::SHEET_LIGHT, "Light", "Items/", "Light", 16);
	load_static(TileSheet::SHEET_TOOL, "Tool", "Items/", "Tool", 16);
	load_static(TileSheet::SHEET_ROCK, "Rock", "Items/", "Rock", 16);
	load_static(TileSheet::SHEET_MUSIC, "Music", "Items/", "Music", 16);
	load_animated(TileSheet::SHEET_CHEST0, "Chest0", "Items/", "Chest", 16);

	// Previously unloaded character sheets
	load_animated(TileSheet::SHEET_SLIME0, "Slime0", "Characters/", "Slime", 16);
	load_animated(TileSheet::SHEET_CAT0, "Cat0", "Characters/", "Cat", 16);
	load_animated(TileSheet::SHEET_RODENT0, "Rodent0", "Characters/", "Rodent", 16);
	load_animated(TileSheet::SHEET_PLANT0, "Plant0", "Characters/", "Plant", 16);
	load_animated(TileSheet::SHEET_ELEMENTAL0, "Elemental0", "Characters/", "Elemental", 16);
	load_animated(TileSheet::SHEET_AQUATIC0, "Aquatic0", "Characters/", "Aquatic", 16);

	// Previously unloaded object sheets
	load_animated(TileSheet::SHEET_ORE0, "Ore0", "Objects/", "Ore", 16);
	load_animated(TileSheet::SHEET_HILL0, "Hill0", "Objects/", "Hill", 16);
	load_animated(TileSheet::SHEET_TREE0, "Tree0", "Objects/", "Tree", 16);
	load_animated(TileSheet::SHEET_GROUND0, "Ground0", "Objects/", "Ground", 16);
	load_animated(TileSheet::SHEET_TRAP0, "Trap0", "Objects/", "Trap", 16);
	load_static(TileSheet::SHEET_FENCE, "Fence", "Objects/", "Fence", 16);
	load_animated(TileSheet::SHEET_MAP0, "Map0", "Objects/", "Map", 16);

	sheetsLoaded = true;
}

void Renderer::load_font(std::string_view fontPath, int size)
{
	fontSize = size;
	gameFont = LoadFontEx(fontPath.data(), size, nullptr, 256);
	fontLoaded = (gameFont.glyphCount > 0);
	if (fontLoaded)
	{
		SetTextureFilter(gameFont.texture, TEXTURE_FILTER_POINT);
	}
}

void Renderer::add_trauma(float amount)
{
	shakeTrauma = std::min(1.0f, shakeTrauma + amount);
}

void Renderer::begin_frame()
{
	// Decay trauma and compute shake offsets
	shakeTrauma = std::max(0.0f, shakeTrauma - GetFrameTime() * 3.0f);
	float magnitude = shakeTrauma * shakeTrauma;
	if (magnitude > 0.01f)
	{
		float time = static_cast<float>(GetTime());
		shakeOffset.x = static_cast<int>(std::sin(time * 57.3f) * magnitude * 8.0f);
		shakeOffset.y = static_cast<int>(std::cos(time * 43.1f) * magnitude * 8.0f);
	}
	else
	{
		shakeOffset = {};
	}

	double now = GetTime();
	if (now - lastAnimToggle >= animInterval)
	{
		currentAnimFrame = 1 - currentAnimFrame;
		lastAnimToggle = now;
	}

	BeginDrawing();
	ClearBackground(RL_BLACK);
}

void Renderer::end_frame()
{
#ifdef EMSCRIPTEN
	// EndDrawing's order is: flush -> swap -> WaitTime -> PollInputEvents.
	// glfwSwapBuffers may yield (emscripten_sleep), so events can fire between
	// swap and PollInputEvents, causing PollInputEvents to see prev=curr=1 and
	// making IsKeyPressed permanently false.
	// Fix: flush -> PollInputEvents -> swap.
	// Events that arrive during the swap yield are captured next frame with prev=0.
	rlDrawRenderBatchActive();
	PollInputEvents();
	SwapScreenBuffer();
#else
	EndDrawing();
#endif
}

void Renderer::begin_light_mask()
{
	if (!lightMaskLoaded)
	{
		return;
	}
	BeginTextureMode(lightMask);
	ClearBackground(RL_BLACK);
}

void Renderer::add_light_quad(int screenX, int screenY, int tileSize, Color tileColor)
{
	if (!lightMaskLoaded)
	{
		return;
	}
	int flippedY = lightMask.texture.height - screenY - tileSize;
	DrawRectangle(screenX, flippedY, tileSize, tileSize, tileColor);
}

void Renderer::apply_light_mask()
{
	if (!lightMaskLoaded)
	{
		return;
	}
	EndTextureMode();
	BeginBlendMode(BLEND_MULTIPLIED);
	DrawTexture(lightMask.texture, 0, 0, RL_WHITE);
	EndBlendMode();
}

void Renderer::draw_tile(Vector2D gridPos, TileRef tile, Color tint) const
{
	assert(sheetsLoaded);
	if (!tile.is_valid())
	{
		return;
	}

	assert(sheet_idx(tile.sheet) < sheets.size());
	const SpriteSheet& sheet = sheets[sheet_idx(tile.sheet)];
	assert(sheet.loaded && sheet.tilesPerRow > 0);

	// Screen position with camera offset
	float destX = static_cast<float>(gridPos.x * tileSize - camera.x);
	float destY = static_cast<float>(gridPos.y * tileSize - camera.y);

	// Cull tiles outside the visible area
	float tileSizeFloat = static_cast<float>(tileSize);
	if (destX + tileSizeFloat < 0.0f || destX >= static_cast<float>(screenWidth) ||
		destY + tileSizeFloat < 0.0f || destY >= static_cast<float>(screenHeight))
	{
		return;
	}

	const Texture2D& texture = (sheet.animated && currentAnimFrame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle srcRect = {
		static_cast<float>(tile.col * sheet.cellSize),
		static_cast<float>(tile.row * sheet.cellSize),
		static_cast<float>(sheet.cellSize),
		static_cast<float>(sheet.cellSize)
	};

	// Destination rect scaled to display tile size
	Rectangle destRect = { destX, destY, tileSizeFloat, tileSizeFloat };

	DrawTexturePro(texture, srcRect, destRect, { 0.0f, 0.0f }, 0.0f, tint);
}

void Renderer::draw_tile_offset(Vector2D gridPos, int pixelOffsetX, int pixelOffsetY, TileRef tile, Color tint) const
{
	assert(sheetsLoaded);
	if (!tile.is_valid())
	{
		return;
	}

	assert(sheet_idx(tile.sheet) < sheets.size());
	const SpriteSheet& sheet = sheets[sheet_idx(tile.sheet)];
	assert(sheet.loaded && sheet.tilesPerRow > 0);

	float destX = static_cast<float>(gridPos.x * tileSize - camera.x + pixelOffsetX);
	float destY = static_cast<float>(gridPos.y * tileSize - camera.y + pixelOffsetY);

	float tileSizeFloat = static_cast<float>(tileSize);
	if (destX + tileSizeFloat < 0.0f || destX >= static_cast<float>(screenWidth) ||
		destY + tileSizeFloat < 0.0f || destY >= static_cast<float>(screenHeight))
	{
		return;
	}

	const Texture2D& texture = (sheet.animated && currentAnimFrame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle srcRect = {
		static_cast<float>(tile.col * sheet.cellSize),
		static_cast<float>(tile.row * sheet.cellSize),
		static_cast<float>(sheet.cellSize),
		static_cast<float>(sheet.cellSize)
	};

	Rectangle destRect = { destX, destY, tileSizeFloat, tileSizeFloat };

	DrawTexturePro(texture, srcRect, destRect, { 0.0f, 0.0f }, 0.0f, tint);
}

void Renderer::draw_tile_static(Vector2D gridPos, TileRef tile, Color tint) const
{
	assert(sheetsLoaded);
	if (!tile.is_valid())
	{
		return;
	}

	assert(sheet_idx(tile.sheet) < sheets.size());
	const SpriteSheet& sheet = sheets[sheet_idx(tile.sheet)];
	assert(sheet.loaded && sheet.tilesPerRow > 0);

	float destX = static_cast<float>(gridPos.x * tileSize - camera.x);
	float destY = static_cast<float>(gridPos.y * tileSize - camera.y);

	float tileSizeFloat = static_cast<float>(tileSize);
	if (destX + tileSizeFloat < 0.0f || destX >= static_cast<float>(screenWidth) ||
		destY + tileSizeFloat < 0.0f || destY >= static_cast<float>(screenHeight))
	{
		return;
	}

	Rectangle srcRect = {
		static_cast<float>(tile.col * sheet.cellSize),
		static_cast<float>(tile.row * sheet.cellSize),
		static_cast<float>(sheet.cellSize),
		static_cast<float>(sheet.cellSize)
	};

	Rectangle destRect = { destX, destY, tileSizeFloat, tileSizeFloat };

	DrawTexturePro(sheet.frame0, srcRect, destRect, { 0.0f, 0.0f }, 0.0f, tint);
}

void Renderer::draw_tile_screen(Vector2D screenPos, TileRef tile) const
{
	assert(sheetsLoaded);
	if (!tile.is_valid())
	{
		return;
	}

	assert(sheet_idx(tile.sheet) < sheets.size());
	const SpriteSheet& sheet = sheets[sheet_idx(tile.sheet)];
	assert(sheet.loaded && sheet.tilesPerRow > 0);

	const Texture2D& texture = (sheet.animated && currentAnimFrame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle srcRect = {
		static_cast<float>(tile.col * sheet.cellSize),
		static_cast<float>(tile.row * sheet.cellSize),
		static_cast<float>(sheet.cellSize),
		static_cast<float>(sheet.cellSize)
	};

	float tileSizeFloat = static_cast<float>(tileSize);
	Rectangle destRect = {
		static_cast<float>(screenPos.x),
		static_cast<float>(screenPos.y),
		tileSizeFloat,
		tileSizeFloat
	};

	DrawTexturePro(texture, srcRect, destRect, { 0.0f, 0.0f }, 0.0f, RL_WHITE);
}

void Renderer::draw_tile_screen_color(Vector2D screenPos, TileRef tile, Color tint) const
{
	assert(sheetsLoaded);
	if (!tile.is_valid())
	{
		return;
	}

	assert(sheet_idx(tile.sheet) < sheets.size());
	const SpriteSheet& sheet = sheets[sheet_idx(tile.sheet)];
	assert(sheet.loaded && sheet.tilesPerRow > 0);

	const Texture2D& texture = (sheet.animated && currentAnimFrame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle srcRect = {
		static_cast<float>(tile.col * sheet.cellSize),
		static_cast<float>(tile.row * sheet.cellSize),
		static_cast<float>(sheet.cellSize),
		static_cast<float>(sheet.cellSize)
	};

	float tileSizeFloat = static_cast<float>(tileSize);
	Rectangle destRect = {
		static_cast<float>(screenPos.x),
		static_cast<float>(screenPos.y),
		tileSizeFloat,
		tileSizeFloat
	};

	DrawTexturePro(texture, srcRect, destRect, { 0.0f, 0.0f }, 0.0f, tint);
}

void Renderer::draw_tile_screen_color_sized(Vector2D screenPos, int size, TileRef tile, Color tint) const
{
	assert(sheetsLoaded);
	if (!tile.is_valid())
	{
		return;
	}

	assert(sheet_idx(tile.sheet) < sheets.size());
	const SpriteSheet& sheet = sheets[sheet_idx(tile.sheet)];
	assert(sheet.loaded && sheet.tilesPerRow > 0);

	const Texture2D& texture = (sheet.animated && currentAnimFrame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle srcRect = {
		static_cast<float>(tile.col * sheet.cellSize),
		static_cast<float>(tile.row * sheet.cellSize),
		static_cast<float>(sheet.cellSize),
		static_cast<float>(sheet.cellSize)
	};

	float sizeFloat = static_cast<float>(size);
	Rectangle destRect = {
		static_cast<float>(screenPos.x),
		static_cast<float>(screenPos.y),
		sizeFloat,
		sizeFloat
	};

	DrawTexturePro(texture, srcRect, destRect, { 0.0f, 0.0f }, 0.0f, tint);
}

void Renderer::draw_tile_screen_sized(Vector2D screenPos, TileRef tile, int displaySize) const
{
	assert(sheetsLoaded);
	if (!tile.is_valid())
	{
		return;
	}

	assert(sheet_idx(tile.sheet) < sheets.size());
	const SpriteSheet& sheet = sheets[sheet_idx(tile.sheet)];
	assert(sheet.loaded && sheet.tilesPerRow > 0);

	const Texture2D& texture = (sheet.animated && currentAnimFrame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle srcRect = {
		static_cast<float>(tile.col * sheet.cellSize),
		static_cast<float>(tile.row * sheet.cellSize),
		static_cast<float>(sheet.cellSize),
		static_cast<float>(sheet.cellSize)
	};

	float displaySizeFloat = static_cast<float>(displaySize);
	Rectangle destRect = {
		static_cast<float>(screenPos.x),
		static_cast<float>(screenPos.y),
		displaySizeFloat,
		displaySizeFloat
	};

	DrawTexturePro(texture, srcRect, destRect, { 0.0f, 0.0f }, 0.0f, RL_WHITE);
}

void Renderer::draw_text(Vector2D screenPos, std::string_view text, int colorPairId) const
{
	ColorPair pair = get_color_pair(colorPairId);
	std::string textStr(text);

	if (fontLoaded)
	{
		Vector2 pos = { static_cast<float>(screenPos.x), static_cast<float>(screenPos.y) };
		DrawTextEx(gameFont, textStr.c_str(), pos, static_cast<float>(fontSize), 1.0f, pair.fg);
	}
	else
	{
		DrawText(textStr.c_str(), screenPos.x, screenPos.y, fontSize, pair.fg);
	}
}

void Renderer::draw_text_color(Vector2D screenPos, std::string_view text, Color color) const
{
	std::string textStr(text);

	if (fontLoaded)
	{
		Vector2 pos = { static_cast<float>(screenPos.x), static_cast<float>(screenPos.y) };
		DrawTextEx(gameFont, textStr.c_str(), pos, static_cast<float>(fontSize), 1.0f, color);
	}
	else
	{
		DrawText(textStr.c_str(), screenPos.x, screenPos.y, fontSize, color);
	}
}

void Renderer::zoom_in()
{
	static constexpr int zoom_levels[] = { 16, 24, 32, 48 };
	for (int i = 0; i < 3; ++i)
	{
		if (tileSize == zoom_levels[i])
		{
			tileSize = zoom_levels[i + 1];
			fontSize = tileSize * 3 / 4;
			update_viewport();
			return;
		}
	}
}

void Renderer::zoom_out()
{
	static constexpr int zoom_levels[] = { 16, 24, 32, 48 };
	for (int i = 1; i < 4; ++i)
	{
		if (tileSize == zoom_levels[i])
		{
			tileSize = zoom_levels[i - 1];
			fontSize = tileSize * 3 / 4;
			update_viewport();
			return;
		}
	}
}

void Renderer::draw_frame(Vector2D screenPos, int wTiles, int hTiles, const TileConfig& tileConfig) const
{
	assert(sheetsLoaded && "Renderer::draw_frame called before sheets are loaded");

	DrawRectangle(screenPos.x, screenPos.y, wTiles * tileSize, hTiles * tileSize, Color{ 8, 8, 16, 255 });

	// Top border
	draw_tile_screen(screenPos, tileConfig.get("GUI_FRAME_TL"));
	for (int col = 1; col < wTiles - 1; ++col)
	{
		draw_tile_screen(Vector2D{ screenPos.x + col * tileSize, screenPos.y }, tileConfig.get("GUI_FRAME_T"));
	}
	draw_tile_screen(Vector2D{ screenPos.x + (wTiles - 1) * tileSize, screenPos.y }, tileConfig.get("GUI_FRAME_TR"));

	// Left and right borders
	for (int row = 1; row < hTiles - 1; ++row)
	{
		draw_tile_screen(Vector2D{ screenPos.x, screenPos.y + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
		draw_tile_screen(Vector2D{ screenPos.x + (wTiles - 1) * tileSize, screenPos.y + row * tileSize }, tileConfig.get("GUI_FRAME_R"));
	}

	// Bottom border
	draw_tile_screen(Vector2D{ screenPos.x, screenPos.y + (hTiles - 1) * tileSize }, tileConfig.get("GUI_FRAME_BL"));
	for (int col = 1; col < wTiles - 1; ++col)
	{
		draw_tile_screen(Vector2D{ screenPos.x + col * tileSize, screenPos.y + (hTiles - 1) * tileSize }, tileConfig.get("GUI_FRAME_B"));
	}
	draw_tile_screen(Vector2D{ screenPos.x + (wTiles - 1) * tileSize, screenPos.y + (hTiles - 1) * tileSize }, tileConfig.get("GUI_FRAME_BR"));
}

void Renderer::draw_bar(Vector2D screenPos, int w, int h, float ratio, Color filled, Color empty) const
{
	DrawRectangle(screenPos.x, screenPos.y, w, h, empty);

	int filledW = static_cast<int>(static_cast<float>(w) * ratio);
	if (filledW > 0)
	{
		DrawRectangle(screenPos.x, screenPos.y, filledW, h, filled);
	}
}

void Renderer::set_camera_center(int world_tile_x, int world_tile_y, int map_w, int map_h)
{
	int map_viewport_rows = viewportRows - GUI_RESERVE_ROWS;
	int viewport_px_w = viewportCols * tileSize;
	int viewport_px_h = map_viewport_rows * tileSize;

	camera.x = world_tile_x * tileSize - viewport_px_w / 2;
	camera.y = world_tile_y * tileSize - viewport_px_h / 2;

	int map_px_w = map_w * tileSize;
	int map_px_h = map_h * tileSize;

	camera.x = std::clamp(camera.x, 0, std::max(0, map_px_w - viewport_px_w));
	camera.y = std::clamp(camera.y, 0, std::max(0, map_px_h - viewport_px_h));
}

ColorPair Renderer::get_color_pair(int id) const
{
	if (id >= 0 && id < MAX_COLOR_PAIRS)
	{
		return colorPairs[id];
	}
	return ColorPair{ RL_WHITE, RL_BLACK };
}

ScreenMetrics Renderer::metrics() const
{
	return ScreenMetrics{
		.tile_w = tileSize,
		.tile_h = tileSize,
		.map_cols = viewportCols,
		.map_rows = viewportRows - GUI_RESERVE_ROWS,
		.gui_rows = GUI_RESERVE_ROWS,
		.window_w = screenWidth,
		.window_h = screenHeight
	};
}

int Renderer::measure_text(std::string_view text) const
{
	std::string text_str(text);
	if (fontLoaded)
	{
		Vector2 size = MeasureTextEx(gameFont, text_str.c_str(), static_cast<float>(fontSize), 1.0f);
		return static_cast<int>(size.x);
	}
	return MeasureText(text_str.c_str(), fontSize);
}

void Renderer::init_color_pairs()
{
	// Default: white on black
	for (auto& pair : colorPairs)
	{
		pair = ColorPair{ RL_WHITE, RL_BLACK };
	}

	// === WHITE FOREGROUND PAIRS ===
	colorPairs[1] = ColorPair{ RL_WHITE, RL_BLACK };
	colorPairs[2] = ColorPair{ RL_WHITE, RL_RED };
	colorPairs[3] = ColorPair{ RL_WHITE, RL_BLUE };
	colorPairs[4] = ColorPair{ RL_WHITE, Color{ 0, 102, 0, 255 } }; // dim green bg

	// === BLACK FOREGROUND PAIRS ===
	colorPairs[5] = ColorPair{ RL_BLACK, RL_WHITE };
	colorPairs[6] = ColorPair{ RL_BLACK, RL_GREEN };
	colorPairs[7] = ColorPair{ RL_BLACK, RL_YELLOW };
	colorPairs[8] = ColorPair{ RL_BLACK, RL_RED };

	// === COLORED FOREGROUND ON BLACK ===
	colorPairs[9] = ColorPair{ RL_RED, RL_BLACK };
	colorPairs[10] = ColorPair{ RL_GREEN, RL_BLACK };
	colorPairs[11] = ColorPair{ RL_YELLOW, RL_BLACK };
	colorPairs[12] = ColorPair{ RL_BLUE, RL_BLACK };
	colorPairs[13] = ColorPair{ Color{ 0, 255, 255, 255 }, RL_BLACK }; // cyan
	colorPairs[14] = ColorPair{ RL_MAGENTA, RL_BLACK };

	// === SPECIAL COMBINATIONS ===
	colorPairs[15] = ColorPair{ Color{ 0, 255, 255, 255 }, RL_BLUE }; // cyan on blue
	colorPairs[16] = ColorPair{ RL_RED, RL_WHITE };
	colorPairs[17] = ColorPair{ RL_GREEN, RL_YELLOW };
	colorPairs[18] = ColorPair{ RL_GREEN, RL_MAGENTA };
	colorPairs[19] = ColorPair{ RL_RED, RL_YELLOW };
	colorPairs[20] = ColorPair{ RL_GREEN, RL_RED };

	// === CUSTOM COLORS ===
	colorPairs[21] = ColorPair{ Color{ 128, 77, 0, 255 }, RL_BLACK }; // brown
	colorPairs[22] = ColorPair{ Color{ 0, 102, 0, 255 }, RL_BLACK }; // dim green
}

// end of file: Renderer.cpp
