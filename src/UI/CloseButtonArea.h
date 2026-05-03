#pragma once

// CloseButtonArea -- hit-test region for a close button drawn in a UI panel.
// Constructed from a Renderer and viewport column count; answers whether a
// given pixel coordinate falls inside the button.

#include "../Renderer/Renderer.h"

class CloseButtonArea
{
private:
	int x{};
	int y{};
	int width{};
	int height{};

public:
	CloseButtonArea(Renderer& renderer, int vcols);

	bool contains(int px, int py) const
	{
		return px >= x && px < x + width && py >= y && py < y + height;
	}

	int get_x() const { return x; }
	int get_y() const { return y; }
};
