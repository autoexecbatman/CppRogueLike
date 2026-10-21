#include "CloseButtonArea.h"
#include "Renderer.h"

CloseButtonArea::CloseButtonArea(Renderer& renderer, int vcols)
{
	int tileSize = renderer.get_tile_size();
	int fontOff = (tileSize - renderer.get_font_size()) / 2;
	int textW = renderer.measure_text("[X]");
	pixelX = (vcols - 2) * tileSize + (tileSize - textW) / 2;
	pixelY = fontOff;
	width = textW;
	height = renderer.get_font_size();
}
