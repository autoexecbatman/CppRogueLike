#pragma once

struct GameContext;

class Minimap
{
	bool visible{ false };

public:
	void toggle() noexcept;
	[[nodiscard]] bool is_visible() const noexcept;
	void render(const GameContext& ctx) const;
};
