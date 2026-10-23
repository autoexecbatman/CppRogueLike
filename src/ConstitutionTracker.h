#pragma once

#include <cstdint>
#include <optional>

class Creature;
class DataManager;

class ConstitutionTracker
{
private:
	// The score whose bonus the owner's hit points already include. Empty until
	// the first application, so a fresh creature is never compared against a
	// score it did not have.
	std::optional<int> lastConstitution{};

public:
	ConstitutionTracker() = default;
	~ConstitutionTracker() = default;
	ConstitutionTracker(const ConstitutionTracker&) = delete;
	ConstitutionTracker(ConstitutionTracker&&) = delete;
	ConstitutionTracker& operator=(const ConstitutionTracker&) = delete;
	ConstitutionTracker& operator=(ConstitutionTracker&&) = delete;

	[[nodiscard]] std::optional<int> get_last_constitution() const noexcept
	{
		return lastConstitution;
	}

	void set_last_constitution(int value) noexcept { lastConstitution = value; }

	struct ConstitutionChangeResult
	{
		bool died{ false };
		int hpDifference{ 0 };
		int oldBonus{ 0 };
		int newBonus{ 0 };
		// True when the bonus was applied for the first time rather than moved:
		// there was no earlier score, so there is no change to report.
		bool firstApplication{ false };
	};

	// Brings the owner's Constitution hit point bonus up to its current score: the change
	// in the table's adjustment, times the levels that take it. Records the score, so a
	// second call with nothing changed reports nothing. It reports and the caller applies.
	//
	// Example, a fighter at Constitution 20 with no earlier score:
	//   tracker.apply_constitution_changes(fighter, dataManager).newBonus;           // -> 5
	//   tracker.apply_constitution_changes(fighter, dataManager).firstApplication;   // -> false, already recorded
	[[nodiscard]] ConstitutionChangeResult apply_constitution_changes(
		Creature& owner,
		const DataManager& dataManager);
};
