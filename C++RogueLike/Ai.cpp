// file: Ai.cpp
#include <iostream>
#include <curses.h>
#include <gsl/util>
#include <gsl/pointers>

#include "Menu.h"
#include "Colors.h"
#include "AiMonster.h"
#include "AiMonsterConfused.h"
#include "AiPlayer.h"

//==AI==
std::shared_ptr<Ai> Ai::create(TCODZip& zip)
{
	const AiType type = gsl::narrow_cast<AiType>(zip.getInt());
	std::shared_ptr<Ai> ai = nullptr;

	switch (type)
	{
	case AiType::PLAYER: {ai = std::make_shared<AiPlayer>(); break; }
	case AiType::MONSTER: {ai = std::make_shared<AiMonster>(); break; }
	case AiType::CONFUSED_MONSTER: {ai = std::make_shared<AiMonsterConfused>(0, nullptr); break; }
	default: {std::cout << "Error: Ai::create() - unknown AiType" << std::endl; exit(-1); }
	}

	if (ai) { ai->load(zip); }
	else { std::cout << "Error: Ai::create() - ai is null" << std::endl; exit(-1); }

	return ai; // TODO: don't return nullptr
}
//====

// end of file: Ai.cpp
