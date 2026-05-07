// T3CommonSkillComponent.cpp

#include "Player/T3CommonSkillComponent.h"

FSkillData* UT3CommonSkillComponent::GetSkillDataByID(int32 SkillID)
{
	return BossSkillDataMap.Find(SkillID);
}

void UT3CommonSkillComponent::ExecuteBossSkill(int32 SkillID)
{
	FSkillData* Data = GetSkillDataByID(SkillID);
	if (!Data) return;

	if (!CanExecuteSkill(*Data)) return;

	StartCooldown(SkillID, *Data);
	OnExecuteBossSkill(SkillID);
}

void UT3CommonSkillComponent::ExecuteBossSkillCompleted(int32 SkillID)
{
	OnExecuteBossSkillCompleted(SkillID);
}
