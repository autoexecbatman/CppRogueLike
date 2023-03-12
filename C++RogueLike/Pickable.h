#ifndef PROJECT_PATH_PICKABLE_H_
#define PROJECT_PATH_PICKABLE_H_
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

//==HEALER==
//==
class Healer : public Pickable
{
public:
	int amountToHeal; // how many hp

	Healer(int amountToHeal);

	bool use(Actor& owner, Actor& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====

//==LIGHTNING_BOLT==
//==
class LightningBolt : public Pickable
{
public:
	int maxRange = 0;
	int damage = 0;
	
	LightningBolt(int range, int damage);

	bool use(Actor& owner, Actor& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====

//==FIREBALL==
//==
class Fireball : public LightningBolt
{
public:
	Fireball(int range, int damage);

	bool use(Actor& owner, Actor& wearer);

	void animation(int x, int y, int maxRange);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====

//==CONFUSER==
//==
class Confuser : public Pickable
{
public:
	int nbTurns = 0;
	int range = 0;

	Confuser(int nbTurns, int range);

	bool use(Actor& owner, Actor& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====

#endif // !PROJECT_PATH_PICKABLE_H_