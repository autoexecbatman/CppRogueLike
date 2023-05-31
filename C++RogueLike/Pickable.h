// file: Pickable.h
#ifndef PICKABLE_H
#define PICKABLE_H

#pragma once


//==PICKABLE==
//==
class Pickable : public Persistent
{
public:
	Pickable() = default;
	virtual ~Pickable() {};

	// defaulted copy constructor and copy assignment operator
	Pickable(const Pickable&) = default;
	Pickable& operator=(const Pickable&) = default;

	// defaulted move constructor and move assignment operator
	Pickable(Pickable&&) noexcept = default;
	Pickable& operator=(Pickable&&) noexcept = default;

	bool pick(Actor& owner, const Actor& wearer);
	void drop(Actor& owner, Actor& wearer);

	virtual bool use(Actor& owner, Actor& wearer);
	/*static Pickable* create(TCODZip& zip);*/
	static std::shared_ptr<Pickable> create(TCODZip& zip);
	
protected:
	enum class PickableType : int
	{
		HEALER, LIGHTNING_BOLT, CONFUSER, FIREBALL
	};
};
//====

#endif // !PICKABLE_H
// end of file: Pickable.h
