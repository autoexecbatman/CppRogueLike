#pragma once
//a class for the entities named Actor
class Actor
{
	
public:
	int y = 0, x = 0;//position on map
	int ch = -47;//ascii code
	int col = 0; //TCODColor col;//color

	Actor(int y, int x, int ch, int col/*const TCODColor& col */);
	void render() const;
};