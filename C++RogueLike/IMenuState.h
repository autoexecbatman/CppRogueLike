#pragma once

class IMenuState
{
public:
	virtual void on_selection() = 0;
	virtual ~IMenuState() = default;
};
