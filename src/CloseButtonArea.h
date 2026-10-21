#pragma once

// CloseButtonArea -- hit-test region for a close button drawn in a UI panel.
// Constructed from a Renderer and viewport column count; answers whether a
// given pixel coordinate falls inside the button.

class Renderer;

class CloseButtonArea
{
private:
	int pixelX{};
	int pixelY{};
	int width{};
	int height{};

public:
	CloseButtonArea(Renderer& renderer, int vcols);

	bool contains(int pointX, int pointY) const
	{
		return pointX >= pixelX && pointX < pixelX + width && pointY >= pixelY && pointY < pixelY + height;
	}

	int get_x() const { return pixelX; }

	int get_y() const { return pixelY; }
};
