#pragma once

struct GameContext;

class IMenuState
{
public:
	virtual void on_selection(GameContext& ctx) = 0;
	IMenuState() = default;
	virtual ~IMenuState() = default;
	IMenuState(const IMenuState&) = delete;
	IMenuState& operator=(const IMenuState&) = delete;
	IMenuState(IMenuState&&) = delete;
	IMenuState& operator=(IMenuState&&) = delete;
};
