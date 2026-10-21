// file: AiRecordRefusalTest.cpp
// An AI record missing a field its save always writes is refused, naming the field.
//
// What it is for. A load that reads a field under contains() quietly leaves the member at
// its initial value when the field is missing, so a broken save comes back as a
// different monster and nothing says so. The fields are found from what each kind's save
// actually wrote, so a field added later with a lenient load fails here without anyone
// adding it to a list. The one field allowed to be absent is named below, with its reason.
//
// Run it, from build/bin/Debug:
//
//     cmake --build build --config Debug --target test_exe
//     ./test_exe.exe --gtest_filter=AiRecordRefusalTest.*

#include <gtest/gtest.h>

#include <cassert>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "src/Ai.h"
#include "src/AiMonster.h"
#include "src/AiMonsterConfused.h"
#include "src/Persistent.h"

namespace
{

// A mimic saves disguiseChangeTime only once it has booked a change, so its absence is a
// real state: the next turn books one.
const std::set<std::string> OPTIONAL_KEYS{ "disguiseChangeTime" };

// One of each kind whose save writes more than its type, built the way the game builds it.
std::vector<std::unique_ptr<Ai>> one_of_each_saving_kind()
{
	std::vector<std::unique_ptr<Ai>> kinds;
	kinds.push_back(Ai::make(AiType::RANGED_MONSTER));
	// The shape Creature::apply_confusion makes: wrapped around the creature's own mind.
	kinds.push_back(std::make_unique<AiMonsterConfused>(5, std::make_unique<AiMonster>()));
	kinds.push_back(Ai::make(AiType::MIMIC));
	kinds.push_back(Ai::make(AiType::SPIDER));
	kinds.push_back(Ai::make(AiType::GIANT_SPIDER));
	kinds.push_back(Ai::make(AiType::WEB_SPINNER));
	return kinds;
}

json saved_record(Ai& ai)
{
	json record;
	ai.save(record);
	return record;
}

} // namespace

// Erasing any one key a save wrote, other than the type and the named optional ones, makes
// the load throw, and the refusal names the key it missed.
TEST(AiRecordRefusalTest, EveryFieldASaveWritesIsRequired)
{
	size_t rows = 0;
	for (const std::unique_ptr<Ai>& ai : one_of_each_saving_kind())
	{
		assert(ai && "one_of_each_saving_kind: built a null Ai");
		const json record = saved_record(*ai);
		const std::string kind = record.at("type").get<std::string>();
		for (const auto& [key, value] : record.items())
		{
			if (key == "type" || OPTIONAL_KEYS.contains(key))
			{
				continue;
			}
			++rows;
			json missing = record;
			missing.erase(key);
			try
			{
				(void)Ai::create(missing);
				ADD_FAILURE() << kind << " loaded without its " << key;
			}
			catch (const json::exception& refusal)
			{
				EXPECT_NE(std::string(refusal.what()).find(key), std::string::npos)
					<< kind << " refused a record without " << key << " but named something else: " << refusal.what();
			}
		}
	}
	EXPECT_EQ(rows, 16u) << "the saves wrote a different set of fields than the ones counted when this was written";
}

// The control: a whole record loads, and saves back exactly as it was.
TEST(AiRecordRefusalTest, ACompleteRecordStillLoads)
{
	for (const std::unique_ptr<Ai>& ai : one_of_each_saving_kind())
	{
		assert(ai && "one_of_each_saving_kind: built a null Ai");
		const json record = saved_record(*ai);
		const std::unique_ptr<Ai> loaded = Ai::create(record);
		EXPECT_EQ(saved_record(*loaded), record) << record.at("type").get<std::string>() << " came back different";
	}
}

// A mimic that has booked no disguise change saves no disguiseChangeTime, and loads back
// still without one.
TEST(AiRecordRefusalTest, AMimicWithNoChangeBookedLoads)
{
	const json record = saved_record(*Ai::make(AiType::MIMIC));
	ASSERT_FALSE(record.contains("disguiseChangeTime")) << "a fresh mimic already booked a change";

	const std::unique_ptr<Ai> loaded = Ai::create(record);

	EXPECT_FALSE(saved_record(*loaded).contains("disguiseChangeTime")) << "the load invented a booked change";
}

// end of file: AiRecordRefusalTest.cpp
