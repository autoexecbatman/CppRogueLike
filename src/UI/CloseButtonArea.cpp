#include "CloseButtonArea.h"

CloseButtonArea::CloseButtonArea(Renderer& renderer, int vcols)
{
	int tileSize = renderer.get_tile_size();
	int fontOff = (tileSize - renderer.get_font_size()) / 2;
	int textW = renderer.measure_text("[X]");
	x = (vcols - 2) * tileSize + (tileSize - textW) / 2;
	y = fontOff;
	width = textW;
	height = renderer.get_font_size();
}
