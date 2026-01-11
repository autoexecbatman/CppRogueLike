#pragma once

struct GameContext;

class IMenuState
{
public:
	virtual void on_selection(GameContext& ctx) = 0;
	virtual ~IMenuState() = default;
};
