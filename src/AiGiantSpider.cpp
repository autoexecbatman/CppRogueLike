#include "AiGiantSpider.h"
#include "Colors.h"
#include "ConstitutionAttributes.h"
#include "Creature.h"
#include "DataManager.h"
#include "GameContext.h"
#include "MessageSystem.h"
#include "SavingThrow.h"

// Table 51's Type A: 15 points on a failed save and none on a made one (Dungeon Master's
// Guide, PDF page 737). The Monstrous Manual fixes the onset of this poison at 15 minutes
// for the large spider, whose poison the huge spider shares (PDF pages 1878-1879), and a
// round is a minute. Its victims save at +1.
constexpr int TYPE_A_DAMAGE = 15;
constexpr int TYPE_A_ONSET_ROUNDS = 15;
constexpr int HUGE_SPIDER_SAVE_BONUS = 1;

AiGiantSpider::AiGiantSpider(int poisonChance)
	: AiSpider(poisonChance)
{
}

void AiGiantSpider::inject_venom(Creature& owner, Creature& target, GameContext& ctx)
{
	const int constitutionAdjustment = ctx.dataManager->constitution_for(target.get_constitution()).PoisonSave;
	if (SavingThrows::is_made(target, SavingThrow::PARALYZATION_POISON_DEATH, HUGE_SPIDER_SAVE_BONUS + constitutionAdjustment, ctx))
	{
		ctx.messageSystem->message(owner.actorData.color, owner.actorData.name);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, " injects venom, and it does not take hold.", true);
		return;
	}

	ctx.messageSystem->message(owner.actorData.color, owner.actorData.name);
	ctx.messageSystem->message(WHITE_RED_PAIR, " injects a venom that will take hold!", true);
	target.take_poison(TYPE_A_ONSET_ROUNDS, TYPE_A_DAMAGE);
}
